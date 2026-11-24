package main

import (
	"context"
	"errors"
	"flag"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/bilolhonahmadjanov/flipperlite/server/internal/api"
	"github.com/bilolhonahmadjanov/flipperlite/server/internal/device"
)

func env(key, fallback string) string {
	if value := os.Getenv(key); value != "" {
		return value
	}
	return fallback
}

func main() {
	deviceURL := flag.String("device", env("FLITE_DEVICE_URL", "http://192.168.4.1"),
		"ESP32 device base URL")
	listen := flag.String("listen", env("FLITE_LISTEN_ADDR", ":8080"),
		"bridge listen address")
	flag.Parse()

	bridge := api.New(device.New(*deviceURL))
	server := &http.Server{
		Addr:              *listen,
		Handler:           bridge.Handler(),
		ReadHeaderTimeout: 5 * time.Second,
	}

	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer stop()
	go func() {
		<-ctx.Done()
		shutdown, cancel := context.WithTimeout(context.Background(), 5*time.Second)
		defer cancel()
		if err := server.Shutdown(shutdown); err != nil {
			log.Printf("shutdown: %v", err)
		}
	}()

	log.Printf("FlipperLite bridge: %s -> %s", *listen, *deviceURL)
	if err := server.ListenAndServe(); err != nil && !errors.Is(err, http.ErrServerClosed) {
		log.Fatal(err)
	}
}
