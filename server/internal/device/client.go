// Package device talks to the HTTP API exposed by the ESP32 firmware.
package device

import (
	"bufio"
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"strings"
	"time"
)

type Status struct {
	Name     string `json:"name"`
	Version  string `json:"version"`
	App      string `json:"app"`
	Mode     string `json:"mode"`
	SSID     string `json:"ssid"`
	IP       string `json:"ip"`
	UptimeS  uint64 `json:"uptime_s"`
	FreeHeap uint64 `json:"free_heap"`
	Display  struct {
		Width  int `json:"width"`
		Height int `json:"height"`
	} `json:"display"`
}

type App struct {
	ID   string `json:"id"`
	Name string `json:"name"`
}

type Event struct {
	Type   string `json:"type"`
	Key    string `json:"key,omitempty"`
	Action string `json:"action,omitempty"`
	Text   string `json:"text,omitempty"`
	TsMS   uint64 `json:"ts_ms"`
}

// API is the device contract used by the HTTP bridge (easy to fake in tests).
type API interface {
	Status(ctx context.Context) (Status, error)
	Apps(ctx context.Context) ([]App, error)
	Display(ctx context.Context) (json.RawMessage, error)
	Launch(ctx context.Context, id string) error
	Input(ctx context.Context, key, action string) error
	ConnectWiFi(ctx context.Context, ssid, password string) error
	Events(ctx context.Context) (<-chan Event, <-chan error)
}

type Client struct {
	baseURL string
	http    *http.Client
	stream  *http.Client
}

func New(baseURL string) *Client {
	baseURL = strings.TrimRight(baseURL, "/")
	return &Client{
		baseURL: baseURL,
		http:    &http.Client{Timeout: 5 * time.Second},
		stream:  &http.Client{}, // SSE connection must stay open
	}
}

func (c *Client) do(ctx context.Context, method, path string, payload, out any) error {
	var body io.Reader
	if payload != nil {
		encoded, err := json.Marshal(payload)
		if err != nil {
			return err
		}
		body = bytes.NewReader(encoded)
	}
	req, err := http.NewRequestWithContext(ctx, method, c.baseURL+path, body)
	if err != nil {
		return err
	}
	if payload != nil {
		req.Header.Set("Content-Type", "application/json")
	}
	resp, err := c.http.Do(req)
	if err != nil {
		return err
	}
	defer resp.Body.Close()
	if resp.StatusCode < 200 || resp.StatusCode >= 300 {
		return fmt.Errorf("device %s: HTTP %d", path, resp.StatusCode)
	}
	if out == nil {
		io.Copy(io.Discard, resp.Body)
		return nil
	}
	return json.NewDecoder(resp.Body).Decode(out)
}

func (c *Client) Status(ctx context.Context) (Status, error) {
	var status Status
	err := c.do(ctx, http.MethodGet, "/api/status", nil, &status)
	return status, err
}

func (c *Client) Apps(ctx context.Context) ([]App, error) {
	var response struct {
		Apps []App `json:"apps"`
	}
	if err := c.do(ctx, http.MethodGet, "/api/apps", nil, &response); err != nil {
		return nil, err
	}
	return response.Apps, nil
}

func (c *Client) Display(ctx context.Context) (json.RawMessage, error) {
	var raw json.RawMessage
	err := c.do(ctx, http.MethodGet, "/api/display", nil, &raw)
	return raw, err
}

func (c *Client) Launch(ctx context.Context, id string) error {
	return c.do(ctx, http.MethodPost, "/api/launch", map[string]string{"id": id}, nil)
}

func (c *Client) Input(ctx context.Context, key, action string) error {
	return c.do(ctx, http.MethodPost, "/api/input", map[string]string{
		"key": key, "action": action,
	}, nil)
}

func (c *Client) ConnectWiFi(ctx context.Context, ssid, password string) error {
	return c.do(ctx, http.MethodPost, "/api/wifi", map[string]string{
		"ssid": ssid, "password": password,
	}, nil)
}

// Events streams the device SSE feed until ctx is cancelled or the device closes it.
func (c *Client) Events(ctx context.Context) (<-chan Event, <-chan error) {
	events := make(chan Event, 16)
	errs := make(chan error, 1)
	req, err := http.NewRequestWithContext(ctx, http.MethodGet, c.baseURL+"/api/events", nil)
	if err != nil {
		errs <- err
		close(events)
		close(errs)
		return events, errs
	}
	resp, err := c.stream.Do(req)
	if err != nil {
		errs <- err
		close(events)
		close(errs)
		return events, errs
	}
	if resp.StatusCode != http.StatusOK {
		resp.Body.Close()
		errs <- fmt.Errorf("device /api/events: HTTP %d", resp.StatusCode)
		close(events)
		close(errs)
		return events, errs
	}
	go func() {
		defer close(events)
		defer close(errs)
		defer resp.Body.Close()
		scanner := bufio.NewScanner(resp.Body)
		scanner.Buffer(make([]byte, 4096), 256*1024)
		for scanner.Scan() {
			line := scanner.Text()
			if !strings.HasPrefix(line, "data: ") {
				continue
			}
			var event Event
			if err := json.Unmarshal([]byte(strings.TrimPrefix(line, "data: ")), &event); err != nil {
				continue
			}
			select {
			case events <- event:
			case <-ctx.Done():
				return
			}
		}
		if err := scanner.Err(); err != nil && ctx.Err() == nil {
			select {
			case errs <- err:
			default:
			}
		}
	}()
	return events, errs
}
