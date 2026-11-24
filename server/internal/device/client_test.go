package device

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"net/http/httptest"
	"testing"
	"time"
)

func TestClientAPI(t *testing.T) {
	var gotInput map[string]string
	srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		switch r.URL.Path {
		case "/api/status":
			fmt.Fprint(w, `{"name":"FlipperLite","version":"0.1.0","app":"menu","mode":"AP","ssid":"FlipperLite-AB","ip":"192.168.4.1","uptime_s":42,"free_heap":100000,"display":{"width":128,"height":64}}`)
		case "/api/apps":
			fmt.Fprint(w, `{"apps":[{"id":"menu","name":"Apps"},{"id":"badge","name":"Badge"}]}`)
		case "/api/display":
			fmt.Fprint(w, `{"width":128,"height":64,"data":"AAAA"}`)
		case "/api/input":
			json.NewDecoder(r.Body).Decode(&gotInput)
			fmt.Fprint(w, `{"ok":true}`)
		case "/api/events":
			w.Header().Set("Content-Type", "text/event-stream")
			fmt.Fprint(w, "retry: 3000\n\nevent: flite\ndata: {\"type\":\"input\",\"key\":\"ok\",\"action\":\"short\"}\n\n")
			w.(http.Flusher).Flush()
			<-r.Context().Done()
		default:
			http.NotFound(w, r)
		}
	}))
	defer srv.Close()

	client := New(srv.URL)
	ctx := context.Background()

	status, err := client.Status(ctx)
	if err != nil || status.Name != "FlipperLite" || status.Display.Width != 128 {
		t.Fatalf("status: %+v, %v", status, err)
	}
	apps, err := client.Apps(ctx)
	if err != nil || len(apps) != 2 || apps[1].ID != "badge" {
		t.Fatalf("apps: %+v, %v", apps, err)
	}
	raw, err := client.Display(ctx)
	if err != nil || len(raw) == 0 {
		t.Fatalf("display: %s, %v", raw, err)
	}
	if err := client.Input(ctx, "ok", "short"); err != nil {
		t.Fatalf("input: %v", err)
	}
	if gotInput["key"] != "ok" || gotInput["action"] != "short" {
		t.Fatalf("input body: %+v", gotInput)
	}

	streamCtx, cancel := context.WithCancel(ctx)
	defer cancel()
	events, errs := client.Events(streamCtx)
	select {
	case event := <-events:
		if event.Type != "input" || event.Key != "ok" {
			t.Fatalf("event: %+v", event)
		}
	case err := <-errs:
		t.Fatalf("event stream error: %v", err)
	case <-time.After(2 * time.Second):
		t.Fatal("event timeout")
	}
}

func TestClientErrorStatus(t *testing.T) {
	srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		http.Error(w, "nope", http.StatusServiceUnavailable)
	}))
	defer srv.Close()
	if _, err := New(srv.URL).Status(context.Background()); err == nil {
		t.Fatal("expected an error for HTTP 503")
	}
}
