package api

import (
	"embed"
	"io/fs"
	"net/http"
)

//go:embed web/*
var webFiles embed.FS

func (s *Server) registerWeb() {
	assets, err := fs.Sub(webFiles, "web")
	if err != nil {
		panic(err)
	}
	fileServer := http.FileServer(http.FS(assets))

	s.mux.HandleFunc("GET /", func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path != "/" {
			http.NotFound(w, r)
			return
		}
		data, err := fs.ReadFile(assets, "index.html")
		if err != nil {
			http.Error(w, "dashboard missing", http.StatusInternalServerError)
			return
		}
		w.Header().Set("Content-Type", "text/html; charset=utf-8")
		w.Header().Set("Cache-Control", "no-cache")
		w.Write(data)
	})
	s.mux.Handle("GET /app.js", fileServer)
	s.mux.Handle("GET /styles.css", fileServer)
	s.mux.Handle("GET /favicon.svg", fileServer)
	s.mux.Handle("GET /manifest.webmanifest", fileServer)
}
