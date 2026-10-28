// Package api assembles every API version the server speaks.
package api

import (
	"log"
	"net/http"
	"time"

	v1 "github.com/everstead/backend/internal/api/v1"
	"github.com/everstead/backend/internal/config"
	"github.com/everstead/backend/internal/httpx"
	"github.com/everstead/backend/internal/store"
)

var Versions = []string{v1.Version}

func Register(mux *http.ServeMux, database *store.Store, settings config.Config) {
	mux.HandleFunc("GET /api", func(writer http.ResponseWriter, _ *http.Request) {
		httpx.WriteJSON(writer, http.StatusOK, map[string]any{
			"name":     "Everstead",
			"versions": Versions,
			"current":  Versions[len(Versions)-1],
		})
	})

	v1.Register(mux, database, settings)
}

func Wrap(handler http.Handler) http.Handler {
	return http.HandlerFunc(func(writer http.ResponseWriter, request *http.Request) {
		started := time.Now()
		defer func() {
			if problem := recover(); problem != nil {
				log.Printf("panic serving %s %s: %v", request.Method, request.URL.Path, problem)
				writer.WriteHeader(http.StatusInternalServerError)
			}
		}()

		handler.ServeHTTP(writer, request)
		log.Printf("%s %s %s %s", httpx.ClientAddress(request), request.Method, request.URL.Path, time.Since(started).Round(time.Millisecond))
	})
}
