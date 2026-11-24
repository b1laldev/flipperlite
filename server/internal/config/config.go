// Package config loads bridge settings from the environment (and optional .env file).
// Secrets are never written to logs or to the repository.
package config

import (
	"bufio"
	"fmt"
	"net"
	"net/url"
	"os"
	"strconv"
	"strings"
)

type Config struct {
	DeviceURL  string
	ListenAddr string
	APIToken   string
	CORSOrigin string
}

func get(key, fallback string) string {
	if value := os.Getenv(key); value != "" {
		return value
	}
	return fallback
}

// LoadDotEnv reads KEY=VALUE pairs from path without overwriting real env vars.
// Missing file is fine; quotes around values are trimmed.
func LoadDotEnv(path string) error {
	file, err := os.Open(path)
	if os.IsNotExist(err) {
		return nil
	}
	if err != nil {
		return err
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		line := strings.TrimSpace(scanner.Text())
		if line == "" || strings.HasPrefix(line, "#") {
			continue
		}
		key, value, found := strings.Cut(line, "=")
		if !found {
			continue
		}
		key = strings.TrimSpace(key)
		value = strings.Trim(strings.TrimSpace(value), `"'`)
		if key == "" || os.Getenv(key) != "" {
			continue
		}
		if err := os.Setenv(key, value); err != nil {
			return err
		}
	}
	return scanner.Err()
}

func Load() (Config, error) {
	config := Config{
		DeviceURL:  get("FLITE_DEVICE_URL", "http://192.168.4.1"),
		ListenAddr: get("FLITE_LISTEN_ADDR", ":8080"),
		APIToken:   os.Getenv("FLITE_API_TOKEN"),
		CORSOrigin: get("FLITE_CORS_ORIGIN", "*"),
	}

	parsed, err := url.Parse(config.DeviceURL)
	if err != nil || parsed.Host == "" || (parsed.Scheme != "http" && parsed.Scheme != "https") {
		return config, fmt.Errorf("FLITE_DEVICE_URL must be an http(s) URL with a host")
	}
	host, port, err := net.SplitHostPort(config.ListenAddr)
	if err != nil {
		return config, fmt.Errorf("FLITE_LISTEN_ADDR must be host:port: %w", err)
	}
	number, err := strconv.Atoi(port)
	if err != nil || number < 1 || number > 65535 {
		return config, fmt.Errorf("FLITE_LISTEN_ADDR port must be 1..65535")
	}
	_ = host // empty host means all interfaces
	if config.APIToken != "" && len(config.APIToken) < 16 {
		return config, fmt.Errorf("FLITE_API_TOKEN must be at least 16 characters")
	}
	return config, nil
}

// SafeHost returns only the device host for logging (credentials are dropped).
func (c Config) SafeHost() string {
	if parsed, err := url.Parse(c.DeviceURL); err == nil {
		return parsed.Host
	}
	return "<invalid>"
}

// TokenEnabled reports whether bearer authentication is required.
func (c Config) TokenEnabled() bool { return c.APIToken != "" }
