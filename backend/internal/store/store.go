// Package store holds every database access: accounts in MongoDB, sessions in PostgreSQL.
package store

import (
	"context"
	"fmt"
	"time"

	"github.com/jackc/pgx/v5/pgxpool"
	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"

	"github.com/everstead/backend/internal/config"
)

type Store struct {
	config config.Config

	mongo    *mongo.Client
	database *mongo.Database
	postgres *pgxpool.Pool
}

func Open(ctx context.Context, cfg config.Config) (*Store, error) {
	mongoClient, err := mongo.Connect(ctx, options.Client().ApplyURI(cfg.MongoURI))
	if err != nil {
		return nil, fmt.Errorf("connect mongo: %w", err)
	}

	if err := mongoClient.Ping(ctx, nil); err != nil {
		return nil, fmt.Errorf("ping mongo: %w", err)
	}

	postgres, err := pgxpool.New(ctx, cfg.PostgresURL)
	if err != nil {
		return nil, fmt.Errorf("connect postgres: %w", err)
	}

	if err := postgres.Ping(ctx); err != nil {
		return nil, fmt.Errorf("ping postgres: %w", err)
	}

	store := &Store{
		config:   cfg,
		mongo:    mongoClient,
		database: mongoClient.Database(cfg.MongoDatabase),
		postgres: postgres,
	}

	if err := store.prepare(ctx); err != nil {
		return nil, err
	}

	if err := store.ensureAdminAccount(ctx); err != nil {
		return nil, fmt.Errorf("ensure admin account: %w", err)
	}

	return store, nil
}

func (s *Store) Close(ctx context.Context) {
	s.postgres.Close()
	_ = s.mongo.Disconnect(ctx)
}

func (s *Store) Config() config.Config {
	return s.config
}

func (s *Store) Health(ctx context.Context) (mongoState string, postgresState string) {
	mongoState = "up"
	if err := s.mongo.Ping(ctx, nil); err != nil {
		mongoState = "down"
	}

	postgresState = "up"
	if err := s.postgres.Ping(ctx); err != nil {
		postgresState = "down"
	}

	return mongoState, postgresState
}

func (s *Store) accounts() *mongo.Collection       { return s.database.Collection("accounts") }
func (s *Store) textures() *mongo.Collection       { return s.database.Collection("textures") }
func (s *Store) counters() *mongo.Collection       { return s.database.Collection("counters") }
func (s *Store) friendships() *mongo.Collection    { return s.database.Collection("friendships") }
func (s *Store) friendRequests() *mongo.Collection { return s.database.Collection("friend_requests") }
func (s *Store) presence() *mongo.Collection       { return s.database.Collection("presence") }
func (s *Store) invites() *mongo.Collection        { return s.database.Collection("invites") }
func (s *Store) gameSessions() *mongo.Collection   { return s.database.Collection("game_sessions") }
func (s *Store) sessionPeers() *mongo.Collection   { return s.database.Collection("session_peers") }
func (s *Store) joinTickets() *mongo.Collection    { return s.database.Collection("join_tickets") }
func (s *Store) forumThreads() *mongo.Collection   { return s.database.Collection("forum_threads") }
func (s *Store) forumPosts() *mongo.Collection     { return s.database.Collection("forum_posts") }
func (s *Store) forumVotes() *mongo.Collection     { return s.database.Collection("forum_votes") }
func (s *Store) forumReports() *mongo.Collection   { return s.database.Collection("forum_reports") }
func (s *Store) modLog() *mongo.Collection         { return s.database.Collection("mod_log") }
func (s *Store) dedicatedServers() *mongo.Collection {
	return s.database.Collection("dedicated_servers")
}

func (s *Store) nextNumber(ctx context.Context, name string, start int64) (int64, error) {
	update := bson.M{"$inc": bson.M{"value": 1}, "$setOnInsert": bson.M{"start": start}}
	options := options.FindOneAndUpdate().SetUpsert(true).SetReturnDocument(options.After)

	var counter struct {
		Value int64 `bson:"value"`
	}

	if err := s.counters().FindOneAndUpdate(ctx, bson.M{"_id": name}, update, options).Decode(&counter); err != nil {
		return 0, err
	}

	return start + counter.Value, nil
}

func now() time.Time {
	return time.Now().UTC()
}
