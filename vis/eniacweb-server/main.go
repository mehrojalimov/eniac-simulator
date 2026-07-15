// Command eniacweb-server is a browser-based front end for the ENIAC
// simulator. It serves a web UI and, for each browser connection, spawns a
// dedicated `eniac -v eniacweb-adapter` process so every session gets its own
// independent, isolated machine (see vis/eniacweb-adapter.go for the other
// half of the bridge).
package main

import (
	"embed"
	"encoding/json"
	"flag"
	"io/fs"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"runtime"
	"sort"
	"strings"
)

//go:embed web
var webFS embed.FS

var (
	addr        = flag.String("addr", ":8080", "HTTP listen address")
	repoRoot    = flag.String("repo-root", ".", "repo root to run eniac from (so relative program/image paths resolve); run this server from the repo root and leave this as \".\"")
	eniacBin    = flag.String("eniac-bin", "", "path to the eniac core binary (default: <repo-root>/src/eniac[.exe])")
	adapterBin  = flag.String("adapter-bin", "", "path to the eniacweb-adapter binary (default: <repo-root>/vis/eniacweb-adapter/eniacweb-adapter[.exe])")
	programsDir = flag.String("programs-dir", "programs", "directory (relative to repo-root) of .e program files")
	maxSessions = flag.Int("max-sessions", 20, "maximum number of concurrent simulator sessions")
)

func exeName(p string) string {
	if runtime.GOOS == "windows" {
		return p + ".exe"
	}
	return p
}

func main() {
	flag.Parse()
	if *eniacBin == "" {
		*eniacBin = filepath.Join(*repoRoot, exeName("src/eniac"))
	}
	if *adapterBin == "" {
		*adapterBin = filepath.Join(*repoRoot, exeName("vis/eniacweb-adapter/eniacweb-adapter"))
	}
	for _, p := range []string{*eniacBin, *adapterBin} {
		if _, err := os.Stat(p); err != nil {
			log.Fatalf("required binary not found: %s (%v). Build it first - see README.md.", p, err)
		}
	}

	sub, err := fs.Sub(webFS, "web")
	if err != nil {
		log.Fatal(err)
	}

	mgr := newSessionManager(*maxSessions)

	mux := http.NewServeMux()
	mux.Handle("/", http.FileServer(http.FS(sub)))
	// Served straight from disk, not embedded: the obj/ 3D model + textures
	// are ~28MB, and shouldn't bloat the server binary or require a rebuild
	// whenever an asset changes.
	mux.Handle("/obj/", http.StripPrefix("/obj/", http.FileServer(http.Dir(filepath.Join(*repoRoot, "obj")))))
	mux.HandleFunc("/api/programs", handlePrograms)
	mux.HandleFunc("/ws", mgr.handleWS)

	log.Printf("eniacweb-server listening on %s (eniac: %s, adapter: %s, programs: %s)",
		*addr, *eniacBin, *adapterBin, filepath.Join(*repoRoot, *programsDir))
	log.Fatal(http.ListenAndServe(*addr, mux))
}

func handlePrograms(w http.ResponseWriter, r *http.Request) {
	dir := filepath.Join(*repoRoot, *programsDir)
	entries, err := os.ReadDir(dir)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	names := make([]string, 0, len(entries))
	for _, e := range entries {
		if !e.IsDir() && strings.HasSuffix(e.Name(), ".e") {
			names = append(names, e.Name())
		}
	}
	sort.Strings(names)
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(names)
}

func validProgramName(name string) bool {
	if name == "" || strings.ContainsAny(name, `/\`) || strings.Contains(name, "..") {
		return false
	}
	info, err := os.Stat(filepath.Join(*repoRoot, *programsDir, name))
	return err == nil && !info.IsDir()
}
