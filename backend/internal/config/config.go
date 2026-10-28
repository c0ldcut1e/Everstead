// Package config reads the server settings from the environment.
package config

import (
	"os"
	"strconv"
	"time"
)

type Config struct {
	ListenAddress     string
	RendezvousAddress string
	PublicHost        string

	DiscourseURL    string
	DiscourseSecret string

	MongoURI      string
	MongoDatabase string
	PostgresURL   string

	AccessTokenTTL  time.Duration
	RefreshTokenTTL time.Duration

	SessionTTL  time.Duration
	PresenceTTL time.Duration
	InviteTTL   time.Duration
	RequestTTL  time.Duration

	MaxRequestBytes int64
}

func Load() Config {
	return Config{
		ListenAddress:     text("EVERSTEAD_LISTEN_ADDRESS", ":8088"),
		RendezvousAddress: text("EVERSTEAD_RENDEZVOUS_ADDRESS", ":37373"),
		PublicHost:        text("EVERSTEAD_PUBLIC_HOST", "127.0.0.1"),

		DiscourseURL:    text("EVERSTEAD_DISCOURSE_URL", "https://forum.plasnet.org"),
		DiscourseSecret: os.Getenv("EVERSTEAD_DISCOURSE_SECRET"),

		MongoURI:      text("EVERSTEAD_MONGO_URI", "mongodb://127.0.0.1:27017"),
		MongoDatabase: text("EVERSTEAD_MONGO_DATABASE", "everstead"),
		PostgresURL:   text("EVERSTEAD_POSTGRES_URL", "postgres://everstead:everstead@127.0.0.1:5432/everstead?sslmode=disable"),

		AccessTokenTTL:  duration("EVERSTEAD_ACCESS_TOKEN_TTL", time.Hour),
		RefreshTokenTTL: duration("EVERSTEAD_REFRESH_TOKEN_TTL", 30*24*time.Hour),

		SessionTTL:  duration("EVERSTEAD_SESSION_TTL", 2*time.Minute),
		PresenceTTL: duration("EVERSTEAD_PRESENCE_TTL", 2*time.Minute),
		InviteTTL:   duration("EVERSTEAD_INVITE_TTL", 5*time.Minute),
		RequestTTL:  duration("EVERSTEAD_FRIEND_REQUEST_TTL", 30*24*time.Hour),

		MaxRequestBytes: number("EVERSTEAD_MAX_REQUEST_BYTES", 1<<20),
	}
}

func text(key string, fallback string) string {
	if value := os.Getenv(key); value != "" {
		return value
	}

	return fallback
}

func duration(key string, fallback time.Duration) time.Duration {
	value, err := time.ParseDuration(os.Getenv(key))
	if err != nil {
		return fallback
	}

	return value
}

func number(key string, fallback int64) int64 {
	value, err := strconv.ParseInt(os.Getenv(key), 10, 64)
	if err != nil {
		return fallback
	}

	return value
}
