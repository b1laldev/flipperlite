package api

import (
	"bufio"
	"context"
	"encoding/json"
	"io"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
	"time"

	"github.com/bilolhonahmadjanov/flipperlite/server/internal/device"
)

type fakeDevice struct {
	launched    string
	inputKey    string
	inputAction string
}

func (f *fakeDevice) Status(context.Context) (device.Status, error) {
	var status device.Status
	json.Unmarshal([]byte(`{"name":"FlipperLite","app":"badge"}`), &status)
	return status, nil
}

func (f *fakeDevice) Apps(context.Context) ([]device.App, error) {
	return []device.App{{ID: "menu", Name: "Apps"}, {ID: "badge", Name: "Badge"}}, nil
}

func (f *fakeDevice) Display(context.Context) (json.RawMessage, error) {
	return json.RawMessage(`{"width":128,"height":64}`), nil
}

func (f *fakeDevice) Launch(_ context.Context, id string) error {
	f.launched = id
	return nil
}

func (f *fakeDevice) Input(_ context.Context, key, action string) error {
	f.inputKey, f.inputAction = key, action
	return nil
}

func (f *fakeDevice) ConnectWiFi(context.Context, string, string) error { return nil }

func (f *fakeDevice) Events(ctx context.Context) (<-chan device.Event, <-chan error) {
	events := make(chan device.Event, 1)
	errs := make(chan error, 1)
	events <- device.Event{Type: "input", Key: "down", Action: "short"}
	go func() { <-ctx.Done(); close(events); close(errs) }()
	return events, errs
}

func request(t *testing.T, client *http.Client, base, method, path, body string) (int, string) {
	t.Helper()
	req, err := http.NewRequest(method, base+path, strings.NewReader(body))
	if err != nil {
		t.Fatal(err)
	}
	req.Header.Set("Content-Type", "application/json")
	resp, err := client.Do(req)
	if err != nil {
		t.Fatal(err)
	}
	defer resp.Body.Close()
	data, err := io.ReadAll(resp.Body)
	if err != nil {
		t.Fatal(err)
	}
	return resp.StatusCode, string(data)
}

func requestWithToken(t *testing.T, client *http.Client, base, method, path, token string) int {
	t.Helper()
	req, err := http.NewRequest(method, base+path, nil)
	if err != nil {
		t.Fatal(err)
	}
	if token != "" {
		req.Header.Set("Authorization", "Bearer "+token)
	}
	resp, err := client.Do(req)
	if err != nil {
		t.Fatal(err)
	}
	defer resp.Body.Close()
	io.Copy(io.Discard, resp.Body)
	return resp.StatusCode
}

func TestAuthAndWeb(t *testing.T) {
	const token = "supersecrettoken123"
	srv := httptest.NewServer(New(&fakeDevice{}, WithToken(token)).Handler())
	defer srv.Close()
	client := srv.Client()

	if status := requestWithToken(t, client, srv.URL, "GET", "/api/v1/status", ""); status != 401 {
		t.Fatalf("unauthenticated request: %d", status)
	}
	if status := requestWithToken(t, client, srv.URL, "GET", "/api/v1/status", token); status != 200 {
		t.Fatalf("authenticated request: %d", status)
	}
	if status := requestWithToken(t, client, srv.URL, "GET", "/health", ""); status != 200 {
		t.Fatalf("health must stay public: %d", status)
	}
	for path, needle := range map[string]string{
		"/":           "FlipperLite",
		"/app.js":     "apiJSON",
		"/styles.css": "--accent",
	} {
		status, body := request(t, client, srv.URL, "GET", path, "")
		if status != 200 || !strings.Contains(body, needle) {
			t.Fatalf("%s: %d, missing %q", path, status, needle)
		}
	}
}

func TestBridge(t *testing.T) {
	fake := &fakeDevice{}
	srv := httptest.NewServer(New(fake).Handler())
	defer srv.Close()
	client := srv.Client()

	if status, body := request(t, client, srv.URL, "GET", "/health", ""); status != 200 ||
		!strings.Contains(body, "ok") {
		t.Fatalf("health: %d %s", status, body)
	}
	if status, body := request(t, client, srv.URL, "GET", "/api/v1/status", ""); status != 200 ||
		!strings.Contains(body, "FlipperLite") {
		t.Fatalf("status: %d %s", status, body)
	}
	if status, body := request(t, client, srv.URL, "GET", "/api/v1/apps", ""); status != 200 ||
		!strings.Contains(body, "badge") {
		t.Fatalf("apps: %d %s", status, body)
	}
	if status, _ := request(t, client, srv.URL, "POST", "/api/v1/input",
		`{"key":"invalid","action":"short"}`); status != 400 {
		t.Fatalf("invalid input should be 400, got %d", status)
	}
	if status, _ := request(t, client, srv.URL, "POST", "/api/v1/input",
		`{"key":"ok","action":"long"}`); status != 200 {
		t.Fatalf("valid input failed: %d", status)
	}
	if fake.inputKey != "ok" || fake.inputAction != "long" {
		t.Fatalf("device got %s/%s", fake.inputKey, fake.inputAction)
	}
	if status, _ := request(t, client, srv.URL, "POST", "/api/v1/launch",
		`{"id":"badge"}`); status != 200 || fake.launched != "badge" {
		t.Fatalf("launch: %d, %s", status, fake.launched)
	}

	resp, err := client.Get(srv.URL + "/api/v1/events")
	if err != nil {
		t.Fatal(err)
	}
	defer resp.Body.Close()
	if resp.StatusCode != 200 || !strings.HasPrefix(resp.Header.Get("Content-Type"), "text/event-stream") {
		t.Fatalf("events: %d %s", resp.StatusCode, resp.Header.Get("Content-Type"))
	}
	deadline := time.After(2 * time.Second)
	scanner := bufio.NewScanner(resp.Body)
	for scanner.Scan() {
		line := scanner.Text()
		if strings.HasPrefix(line, "data: ") && strings.Contains(line, "down") {
			return
		}
		select {
		case <-deadline:
			t.Fatal("SSE event timeout")
		default:
		}
	}
	t.Fatalf("SSE stream ended: %v", scanner.Err())
}
