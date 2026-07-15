// Command eniacweb-adapter is a "vis" program (see the -v flag documented in
// the README) meant to be spawned by the ENIAC core simulator on behalf of
// eniacweb-server. Unlike the other visualizers (eniactk, eniacfp, ...) it
// does not render anything itself: it just bridges the engine display
// protocol (its own stdin/stdout, wired up by the core's enginedisplay()) to
// a loopback TCP connection back to eniacweb-server, which does the actual
// rendering in the browser.
//
// eniacweb-server listens on an OS-assigned port and spawns
// `eniac -v "eniacweb-adapter -port <port>" <program>` per browser session;
// this program dials that port, tells the core to start sending state in
// "Timed" mode, and then just copies bytes in both directions until either
// side closes. When eniacweb-server tears a session down it closes both the
// TCP connection and the eniac process, which is enough to unblock both
// copies below and let this process exit on its own.
package main

import (
	"flag"
	"fmt"
	"io"
	"net"
	"os"
	"sync"
	"time"
)

func main() {
	port := flag.Int("port", 0, "TCP port on 127.0.0.1 to connect back to eniacweb-server")
	flag.Parse()
	if *port <= 0 {
		fmt.Fprintln(os.Stderr, "eniacweb-adapter: -port is required")
		os.Exit(2)
	}

	// Selects "Timed" display mode in engine.go: the core pushes state on a
	// fixed interval rather than waiting for an "update" handshake reply,
	// which keeps this adapter a dumb pipe with no protocol logic of its own.
	fmt.Println("ready")

	var conn net.Conn
	var err error
	addr := fmt.Sprintf("127.0.0.1:%d", *port)
	for attempt := 0; attempt < 20; attempt++ {
		conn, err = net.Dial("tcp", addr)
		if err == nil {
			break
		}
		time.Sleep(50 * time.Millisecond)
	}
	if err != nil {
		fmt.Fprintf(os.Stderr, "eniacweb-adapter: connecting to %s: %v\n", addr, err)
		os.Exit(1)
	}
	defer conn.Close()

	var wg sync.WaitGroup
	wg.Add(2)
	go func() {
		defer wg.Done()
		io.Copy(conn, os.Stdin)
	}()
	go func() {
		defer wg.Done()
		io.Copy(os.Stdout, conn)
	}()
	wg.Wait()
}
