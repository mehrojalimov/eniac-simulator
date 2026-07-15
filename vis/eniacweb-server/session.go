package main

import (
	"bufio"
	"fmt"
	"log"
	"net"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"sync"
	"time"

	"github.com/gorilla/websocket"
)

var upgrader = websocket.Upgrader{
	ReadBufferSize:  4096,
	WriteBufferSize: 4096,
	CheckOrigin:     func(r *http.Request) bool { return true },
}

// sessionManager caps how many simulator processes can run concurrently, so
// a burst of browser connections can't exhaust the host's resources.
type sessionManager struct {
	mu      sync.Mutex
	count   int
	maxSize int
}

func newSessionManager(maxSize int) *sessionManager {
	return &sessionManager{maxSize: maxSize}
}

func (m *sessionManager) acquire() bool {
	m.mu.Lock()
	defer m.mu.Unlock()
	if m.count >= m.maxSize {
		return false
	}
	m.count++
	return true
}

func (m *sessionManager) release() {
	m.mu.Lock()
	m.count--
	m.mu.Unlock()
}

func (m *sessionManager) handleWS(w http.ResponseWriter, r *http.Request) {
	program := r.URL.Query().Get("program")
	if !validProgramName(program) {
		http.Error(w, "missing or invalid \"program\" query parameter", http.StatusBadRequest)
		return
	}
	if !m.acquire() {
		http.Error(w, "server busy: too many active sessions, try again shortly", http.StatusServiceUnavailable)
		return
	}
	defer m.release()

	wsConn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Printf("websocket upgrade failed: %v", err)
		return
	}
	defer wsConn.Close()

	sess, err := startSession(program)
	if err != nil {
		log.Printf("session start failed: %v", err)
		wsConn.WriteMessage(websocket.TextMessage, []byte("error starting simulator: "+err.Error()))
		return
	}
	defer sess.Close()

	log.Printf("session %s started (program=%s)", sess.id, program)
	runSessionBridge(wsConn, sess)
	log.Printf("session %s ended", sess.id)
}

// session is one isolated `eniac` core process plus the eniacweb-adapter
// child it spawned, bridged to us over a loopback TCP connection.
type session struct {
	id   string
	cmd  *exec.Cmd
	conn net.Conn

	mu     sync.Mutex
	closed bool
}

func startSession(program string) (*session, error) {
	ln, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		return nil, fmt.Errorf("listening for adapter callback: %w", err)
	}
	defer ln.Close()
	port := ln.Addr().(*net.TCPAddr).Port

	programPath := filepath.Join(*programsDir, program)
	adapterArg := fmt.Sprintf("%s -port %d", *adapterBin, port)
	cmd := exec.Command(*eniacBin, "-v", adapterArg, programPath)
	cmd.Dir = *repoRoot
	cmd.Stderr = os.Stderr
	if err := cmd.Start(); err != nil {
		return nil, fmt.Errorf("starting eniac: %w", err)
	}

	type acceptResult struct {
		conn net.Conn
		err  error
	}
	acceptCh := make(chan acceptResult, 1)
	go func() {
		c, err := ln.Accept()
		acceptCh <- acceptResult{c, err}
	}()

	select {
	case res := <-acceptCh:
		if res.err != nil {
			cmd.Process.Kill()
			cmd.Wait()
			return nil, fmt.Errorf("accepting adapter connection: %w", res.err)
		}
		return &session{id: fmt.Sprintf("%d", cmd.Process.Pid), cmd: cmd, conn: res.conn}, nil
	case <-time.After(5 * time.Second):
		cmd.Process.Kill()
		cmd.Wait()
		return nil, fmt.Errorf("timed out waiting for eniacweb-adapter to connect back")
	}
}

func (s *session) Close() {
	s.mu.Lock()
	if s.closed {
		s.mu.Unlock()
		return
	}
	s.closed = true
	s.mu.Unlock()

	s.conn.Close()
	if s.cmd.Process != nil {
		s.cmd.Process.Kill()
	}
	s.cmd.Wait()
}

// runSessionBridge copies newline-delimited protocol lines between the
// session's TCP connection and the browser's WebSocket until either side
// closes. It returns once both directions have stopped.
//
// Ending either direction closes both the TCP connection and the WebSocket
// (via "stop", guarded by sync.Once) so the *other* direction's blocking
// read is guaranteed to unblock too - without this, a browser disconnect
// would leave the tcp->ws goroutine blocked forever waiting on data from a
// still-running simulator, and wg.Wait() below would never return.
func runSessionBridge(wsConn *websocket.Conn, sess *session) {
	var wg sync.WaitGroup
	wg.Add(2)

	var once sync.Once
	stop := func() {
		once.Do(func() {
			sess.conn.Close()
			wsConn.Close()
		})
	}

	go func() {
		defer wg.Done()
		defer stop()
		defer func() {
			if r := recover(); r != nil {
				log.Printf("session %s: tcp->ws panic: %v", sess.id, r)
			}
		}()
		scanner := bufio.NewScanner(sess.conn)
		scanner.Buffer(make([]byte, 0, 4096), 1<<20)
		for scanner.Scan() {
			if err := wsConn.WriteMessage(websocket.TextMessage, scanner.Bytes()); err != nil {
				return
			}
		}
	}()

	go func() {
		defer wg.Done()
		defer stop()
		defer func() {
			if r := recover(); r != nil {
				log.Printf("session %s: ws->tcp panic: %v", sess.id, r)
			}
		}()
		for {
			_, msg, err := wsConn.ReadMessage()
			if err != nil {
				return
			}
			msg = append(msg, '\n')
			if _, err := sess.conn.Write(msg); err != nil {
				return
			}
		}
	}()

	wg.Wait()
}
