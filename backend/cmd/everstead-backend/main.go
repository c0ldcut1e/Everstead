// Command everstead-backend runs the account API, the account website and the
// rendezvous that introduces players to hosts.
package main

import (
	"context"
	"errors"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/everstead/backend/internal/api"
	"github.com/everstead/backend/internal/config"
	"github.com/everstead/backend/internal/rendezvous"
	"github.com/everstead/backend/internal/store"
	"github.com/everstead/backend/internal/website"
)

func main() {
	log.SetFlags(log.LstdFlags | log.LUTC)

	settings := config.Load()

	startup, cancelStartup := context.WithTimeout(context.Background(), 30*time.Second)
	defer cancelStartup()

	database, err := store.Open(startup, settings)
	if err != nil {
		log.Fatalf("open databases: %v", err)
	}

	mux := http.NewServeMux()
	api.Register(mux, database, settings)
	website.Register(mux, database, settings)

	httpServer := &http.Server{
		Addr:              settings.ListenAddress,
		Handler:           api.Wrap(mux),
		ReadHeaderTimeout: 10 * time.Second,
		IdleTimeout:       2 * time.Minute,
	}

	introducer, err := rendezvous.Listen(settings.RendezvousAddress, database)
	if err != nil {
		log.Fatalf("start rendezvous: %v", err)
	}

	go func() {
		log.Printf("rendezvous listening on udp %s", introducer.Address())
		if err := introducer.Serve(); err != nil {
			log.Printf("rendezvous stopped: %v", err)
		}
	}()

	go func() {
		log.Printf("api and website listening on %s", settings.ListenAddress)
		if err := httpServer.ListenAndServe(); err != nil && !errors.Is(err, http.ErrServerClosed) {
			log.Fatalf("serve http: %v", err)
		}
	}()

	signals := make(chan os.Signal, 1)
	signal.Notify(signals, syscall.SIGINT, syscall.SIGTERM)
	<-signals

	log.Print("shutting down")

	shutdown, cancelShutdown := context.WithTimeout(context.Background(), 15*time.Second)
	defer cancelShutdown()

	_ = httpServer.Shutdown(shutdown)
	_ = introducer.Close()
	database.Close(shutdown)
}
