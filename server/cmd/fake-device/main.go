// Fake device implements the ESP32 HTTP protocol so the bridge and Android app
// can be exercised without hardware. Never exposes real network credentials.
package main

import (
	"encoding/base64"
	"encoding/json"
	"flag"
	"fmt"
	"log"
	"net/http"
	"sync"
	"time"
)

type hub struct {
	mu   sync.Mutex
	subs map[chan string]bool
	app  string
}

func (h *hub) publish(event map[string]any) {
	data, _ := json.Marshal(event)
	h.mu.Lock()
	defer h.mu.Unlock()
	for ch := range h.subs {
		select {
		case ch <- string(data):
		default:
		}
	}
}

func (h *hub) subscribe() chan string {
	ch := make(chan string, 16)
	h.mu.Lock()
	h.subs[ch] = true
	h.mu.Unlock()
	return ch
}

func (h *hub) unsubscribe(ch chan string) {
	h.mu.Lock()
	delete(h.subs, ch)
	h.mu.Unlock()
	close(ch)
}

func writeJSON(w http.ResponseWriter, value any) {
	w.Header().Set("Access-Control-Allow-Origin", "*")
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(value)
}

func main() {
	listen := flag.String("listen", ":9090", "fake device listen address")
	flag.Parse()

	h := &hub{subs: make(map[chan string]bool), app: "menu"}
	mux := http.NewServeMux()

	mux.HandleFunc("/api/status", func(w http.ResponseWriter, r *http.Request) {
		h.mu.Lock()
		app := h.app
		h.mu.Unlock()
		writeJSON(w, map[string]any{
			"name": "FlipperLite-FAKE", "version": "0.1.0", "app": app,
			"mode": "AP", "ssid": "LAB-WIFI-TEST", "ip": "192.168.4.1",
			"uptime_s": int(time.Since(started).Seconds()), "free_heap": 187000,
			"display": map[string]int{"width": 128, "height": 64},
		})
	})
	mux.HandleFunc("/api/apps", func(w http.ResponseWriter, r *http.Request) {
		writeJSON(w, map[string]any{"apps": []map[string]string{
			{"id": "menu", "name": "Apps"},
			{"id": "badge", "name": "Badge"},
			{"id": "wifi", "name": "WiFi"},
			{"id": "settings", "name": "Settings"},
		}})
	})
	mux.HandleFunc("/api/display", func(w http.ResponseWriter, r *http.Request) {
		frame := make([]byte, 128*8)
		for i := range frame {
			frame[i] = 0x0F
		}
		writeJSON(w, map[string]any{
			"width": 128, "height": 64, "format": "1bit-row-major",
			"data": base64.StdEncoding.EncodeToString(frame),
		})
	})
	mux.HandleFunc("/api/input", func(w http.ResponseWriter, r *http.Request) {
		var request map[string]string
		if json.NewDecoder(r.Body).Decode(&request) != nil {
			http.Error(w, "bad JSON", http.StatusBadRequest)
			return
		}
		h.publish(map[string]any{
			"type": "input", "key": request["key"], "action": request["action"],
			"ts_ms": time.Now().UnixMilli(),
		})
		writeJSON(w, map[string]bool{"ok": true})
	})
	mux.HandleFunc("/api/launch", func(w http.ResponseWriter, r *http.Request) {
		var request map[string]string
		if json.NewDecoder(r.Body).Decode(&request) != nil {
			http.Error(w, "bad JSON", http.StatusBadRequest)
			return
		}
		h.mu.Lock()
		h.app = request["id"]
		h.mu.Unlock()
		h.publish(map[string]any{"type": "app", "text": request["id"], "ts_ms": time.Now().UnixMilli()})
		writeJSON(w, map[string]bool{"ok": true})
	})
	mux.HandleFunc("/api/wifi", func(w http.ResponseWriter, r *http.Request) {
		writeJSON(w, map[string]bool{"ok": true}) // demo only, stores nothing
	})
	mux.HandleFunc("/api/events", func(w http.ResponseWriter, r *http.Request) {
		flusher, ok := w.(http.Flusher)
		if !ok {
			http.Error(w, "no streaming", http.StatusInternalServerError)
			return
		}
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Content-Type", "text/event-stream")
		fmt.Fprint(w, "retry: 3000\n\n")
		flusher.Flush()
		ch := h.subscribe()
		defer h.unsubscribe(ch)
		for {
			select {
			case <-r.Context().Done():
				return
			case data := <-ch:
				fmt.Fprintf(w, "event: flite\ndata: %s\n\n", data)
				flusher.Flush()
			case <-time.After(15 * time.Second):
				fmt.Fprint(w, ": ping\n\n")
				flusher.Flush()
			}
		}
	})

	log.Printf("fake FlipperLite device on %s", *listen)
	log.Fatal(http.ListenAndServe(*listen, mux))
}

var started = time.Now()
