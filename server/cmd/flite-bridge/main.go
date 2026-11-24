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
	"github.com/bilolhonahmadjanov/flipperlite/server/internal/config"
	"github.com/bilolhonahmadjanov/flipperlite/server/internal/device"
)

func main() {
	if err := config.LoadDotEnv(".env"); err != nil {
		log.Printf("warning: could not read .env: %v", err)
	}
	settings, err := config.Load()
	if err != nil {
		log.Fatal(err)
	}

	deviceURL := flag.String("device", settings.DeviceURL, "ESP32 device base URL")
	listen := flag.String("listen", settings.ListenAddr, "bridge listen address")
	flag.Parse()

	dev := device.New(*deviceURL)
	bridge := api.New(dev,
		api.WithToken(settings.APIToken),
		api.WithCORSOrigin(settings.CORSOrigin),
	)
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

	// Only the device host is logged; tokens/passwords never are.
	parsed := config.Config{DeviceURL: *deviceURL}
	log.Printf("FlipperLite bridge: %s -> %s (auth=%t, dashboard at /)",
		*listen, parsed.SafeHost(), settings.TokenEnabled())
	if err := server.ListenAndServe(); err != nil && !errors.Is(err, http.ErrServerClosed) {
		log.Fatal(err)
	}
}
