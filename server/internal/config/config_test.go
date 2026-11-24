package config

import (
	"os"
	"path/filepath"
	"testing"
)

func TestLoadDefaults(t *testing.T) {
	for _, key := range []string{"FLITE_DEVICE_URL", "FLITE_LISTEN_ADDR", "FLITE_API_TOKEN", "FLITE_CORS_ORIGIN"} {
		os.Unsetenv(key)
	}
	config, err := Load()
	if err != nil {
		t.Fatal(err)
	}
	if config.DeviceURL != "http://192.168.4.1" || config.ListenAddr != ":8080" || config.TokenEnabled() {
		t.Fatalf("unexpected defaults: %+v", config)
	}
	if config.SafeHost() != "192.168.4.1" {
		t.Fatalf("safe host: %s", config.SafeHost())
	}
}

func TestLoadValidation(t *testing.T) {
	t.Setenv("FLITE_DEVICE_URL", "not-a-url")
	if _, err := Load(); err == nil {
		t.Fatal("expected invalid device URL error")
	}
	t.Setenv("FLITE_DEVICE_URL", "http://localhost:9090")
	t.Setenv("FLITE_LISTEN_ADDR", ":70000")
	if _, err := Load(); err == nil {
		t.Fatal("expected invalid port error")
	}
	t.Setenv("FLITE_LISTEN_ADDR", ":8080")
	t.Setenv("FLITE_API_TOKEN", "short")
	if _, err := Load(); err == nil {
		t.Fatal("expected short token error")
	}
}

func TestLoadDotEnv(t *testing.T) {
	t.Setenv("FLITE_TEST_VALUE", "from-env")
	path := filepath.Join(t.TempDir(), ".env")
	content := "# comment\nFLITE_TEST_VALUE=from-file\nFLITE_TEST_OTHER=\"quoted\"\n"
	if err := os.WriteFile(path, []byte(content), 0o600); err != nil {
		t.Fatal(err)
	}
	if err := LoadDotEnv(path); err != nil {
		t.Fatal(err)
	}
	if os.Getenv("FLITE_TEST_VALUE") != "from-env" {
		t.Fatal("real env var must win over .env")
	}
	if os.Getenv("FLITE_TEST_OTHER") != "quoted" {
		t.Fatal("quoted .env value not parsed")
	}
}
