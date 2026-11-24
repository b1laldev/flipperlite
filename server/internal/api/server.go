// Package api exposes the phone-facing REST/SSE bridge and the web dashboard.
package api

import (
	"context"
	"crypto/subtle"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"strings"
	"time"

	"github.com/bilolhonahmadjanov/flipperlite/server/internal/device"
)

type Server struct {
	device     device.API
	mux        *http.ServeMux
	token      string
	corsOrigin string
}

type Option func(*Server)

// WithToken enables bearer-token authentication for /api/v1/* endpoints.
func WithToken(token string) Option {
	return func(s *Server) { s.token = token }
}

// WithCORSOrigin restricts browser clients to a specific origin (default "*").
func WithCORSOrigin(origin string) Option {
	return func(s *Server) {
		if origin != "" {
			s.corsOrigin = origin
		}
	}
}

func New(dev device.API, opts ...Option) *Server {
	s := &Server{device: dev, mux: http.NewServeMux(), corsOrigin: "*"}
	for _, opt := range opts {
		opt(s)
	}
	s.mux.HandleFunc("GET /health", s.health)
	s.mux.HandleFunc("GET /api/v1/status", s.status)
	s.mux.HandleFunc("GET /api/v1/apps", s.apps)
	s.mux.HandleFunc("GET /api/v1/display", s.display)
	s.mux.HandleFunc("POST /api/v1/input", s.input)
	s.mux.HandleFunc("POST /api/v1/launch", s.launch)
	s.mux.HandleFunc("POST /api/v1/wifi", s.wifi)
	s.mux.HandleFunc("GET /api/v1/events", s.events)
	s.registerWeb()
	return s
}

func publicPath(path string) bool {
	switch path {
	case "/", "/health", "/app.js", "/styles.css", "/favicon.svg":
		return true
	}
	return false
}

func authorized(header, token string) bool {
	if token == "" {
		return true
	}
	value := strings.TrimSpace(header)
	if !strings.HasPrefix(value, "Bearer ") {
		return false
	}
	provided := strings.TrimSpace(strings.TrimPrefix(value, "Bearer "))
	return subtle.ConstantTimeCompare([]byte(provided), []byte(token)) == 1
}

func (s *Server) Handler() http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", s.corsOrigin)
		w.Header().Set("Access-Control-Allow-Methods", "GET,POST,OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization")
		w.Header().Add("Vary", "Origin")
		if r.Method == http.MethodOptions {
			w.WriteHeader(http.StatusNoContent)
			return
		}
		if !publicPath(r.URL.Path) && !authorized(r.Header.Get("Authorization"), s.token) {
			fail(w, http.StatusUnauthorized, "bearer token required")
			return
		}
		s.mux.ServeHTTP(w, r)
	})
}

func writeJSON(w http.ResponseWriter, status int, value any) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	json.NewEncoder(w).Encode(value)
}

func fail(w http.ResponseWriter, status int, format string, args ...any) {
	writeJSON(w, status, map[string]string{"error": fmt.Sprintf(format, args...)})
}

func decode(w http.ResponseWriter, r *http.Request, out any) bool {
	if err := json.NewDecoder(http.MaxBytesReader(w, r.Body, 4096)).Decode(out); err != nil {
		fail(w, http.StatusBadRequest, "invalid JSON: %v", err)
		return false
	}
	return true
}

func (s *Server) health(w http.ResponseWriter, r *http.Request) {
	writeJSON(w, http.StatusOK, map[string]string{
		"status": "ok", "service": "flite-bridge", "auth": fmt.Sprintf("%t", s.token != ""),
	})
}

func (s *Server) status(w http.ResponseWriter, r *http.Request) {
	status, err := s.device.Status(r.Context())
	if err != nil {
		fail(w, http.StatusBadGateway, "device unreachable: %v", err)
		return
	}
	writeJSON(w, http.StatusOK, status)
}

func (s *Server) apps(w http.ResponseWriter, r *http.Request) {
	apps, err := s.device.Apps(r.Context())
	if err != nil {
		fail(w, http.StatusBadGateway, "device unreachable: %v", err)
		return
	}
	writeJSON(w, http.StatusOK, map[string]any{"apps": apps})
}

func (s *Server) display(w http.ResponseWriter, r *http.Request) {
	raw, err := s.device.Display(r.Context())
	if err != nil {
		fail(w, http.StatusBadGateway, "device unreachable: %v", err)
		return
	}
	w.Header().Set("Content-Type", "application/json")
	w.Write(raw)
}

var allowedKeys = map[string]bool{
	"up": true, "down": true, "left": true, "right": true, "ok": true, "back": true,
}
var allowedActions = map[string]bool{
	"short": true, "long": true, "repeat": true,
}

func (s *Server) input(w http.ResponseWriter, r *http.Request) {
	var request struct {
		Key    string `json:"key"`
		Action string `json:"action"`
	}
	if !decode(w, r, &request) {
		return
	}
	if !allowedKeys[request.Key] || !allowedActions[request.Action] {
		fail(w, http.StatusBadRequest, "unknown key or action")
		return
	}
	if err := s.device.Input(r.Context(), request.Key, request.Action); err != nil {
		fail(w, http.StatusBadGateway, "device error: %v", err)
		return
	}
	writeJSON(w, http.StatusOK, map[string]bool{"ok": true})
}

func (s *Server) launch(w http.ResponseWriter, r *http.Request) {
	var request struct {
		ID string `json:"id"`
	}
	if !decode(w, r, &request) || request.ID == "" {
		return
	}
	if err := s.device.Launch(r.Context(), request.ID); err != nil {
		fail(w, http.StatusBadGateway, "device error: %v", err)
		return
	}
	writeJSON(w, http.StatusOK, map[string]bool{"ok": true})
}

func (s *Server) wifi(w http.ResponseWriter, r *http.Request) {
	var request struct {
		SSID     string `json:"ssid"`
		Password string `json:"password"`
	}
	if !decode(w, r, &request) || request.SSID == "" {
		return
	}
	if err := s.device.ConnectWiFi(r.Context(), request.SSID, request.Password); err != nil {
		fail(w, http.StatusBadGateway, "device error: %v", err)
		return
	}
	writeJSON(w, http.StatusOK, map[string]bool{"ok": true})
}

func (s *Server) events(w http.ResponseWriter, r *http.Request) {
	flusher, ok := w.(http.Flusher)
	if !ok {
		fail(w, http.StatusInternalServerError, "streaming unsupported")
		return
	}
	w.Header().Set("Content-Type", "text/event-stream")
	w.Header().Set("Cache-Control", "no-cache")
	w.Header().Set("Connection", "keep-alive")
	fmt.Fprint(w, "retry: 3000\n\n")
	flusher.Flush()

	ctx, cancel := context.WithCancel(r.Context())
	defer cancel()
	events, errs := s.device.Events(ctx)
	keepalive := time.NewTicker(15 * time.Second)
	defer keepalive.Stop()
	for {
		select {
		case <-r.Context().Done():
			return
		case event, ok := <-events:
			if !ok {
				return
			}
			data, _ := json.Marshal(event)
			fmt.Fprintf(w, "event: flite\ndata: %s\n\n", data)
			flusher.Flush()
		case err := <-errs:
			if err != nil {
				log.Printf("device event stream: %v", err)
				data, _ := json.Marshal(map[string]string{"error": err.Error()})
				fmt.Fprintf(w, "event: error\ndata: %s\n\n", data)
				flusher.Flush()
			}
			return
		case <-keepalive.C:
			fmt.Fprint(w, ": ping\n\n")
			flusher.Flush()
		}
	}
}
