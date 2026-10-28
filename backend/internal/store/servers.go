package store

import (
	"context"
	"errors"
	"strings"
	"time"

	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"
)

var ErrServerLimit = errors.New("store: server limit reached")

const MaxServersPerAccount = 2

type DedicatedServer struct {
	ServerID  string    `bson:"serverId"  json:"serverId"`
	OwnerPid  uint32    `bson:"ownerPid"  json:"ownerPid"`
	Name      string    `bson:"name"      json:"name"`
	NameLower string    `bson:"nameLower" json:"-"`
	TokenHash string    `bson:"tokenHash" json:"-"`
	CreatedAt time.Time `bson:"createdAt" json:"-"`
}

func (s *Store) CreateDedicatedServer(ctx context.Context, ownerPid uint32, name string, tokenHash string) (*DedicatedServer, error) {
	count, err := s.dedicatedServers().CountDocuments(ctx, bson.M{"ownerPid": ownerPid})
	if err != nil {
		return nil, err
	}

	if count >= MaxServersPerAccount {
		return nil, ErrServerLimit
	}

	server := &DedicatedServer{
		ServerID:  randomText(8),
		OwnerPid:  ownerPid,
		Name:      name,
		NameLower: strings.ToLower(name),
		TokenHash: tokenHash,
		CreatedAt: now(),
	}

	if _, err := s.dedicatedServers().InsertOne(ctx, server); err != nil {
		if mongo.IsDuplicateKeyError(err) {
			return nil, ErrConflict
		}

		return nil, err
	}

	return server, nil
}

func (s *Store) DedicatedServersByOwner(ctx context.Context, ownerPid uint32) ([]DedicatedServer, error) {
	servers := []DedicatedServer{}
	cursor, err := s.dedicatedServers().Find(ctx, bson.M{"ownerPid": ownerPid})
	if err != nil {
		return nil, err
	}

	defer cursor.Close(ctx)

	if err := cursor.All(ctx, &servers); err != nil {
		return nil, err
	}

	return servers, nil
}

func (s *Store) DedicatedServerByName(ctx context.Context, name string) (*DedicatedServer, error) {
	return s.findDedicatedServer(ctx, bson.M{"nameLower": strings.ToLower(name)})
}

func (s *Store) DedicatedServerByTokenHash(ctx context.Context, tokenHash string) (*DedicatedServer, error) {
	return s.findDedicatedServer(ctx, bson.M{"tokenHash": tokenHash})
}

func (s *Store) findDedicatedServer(ctx context.Context, filter bson.M) (*DedicatedServer, error) {
	server := &DedicatedServer{}
	err := s.dedicatedServers().FindOne(ctx, filter).Decode(server)
	if errors.Is(err, mongo.ErrNoDocuments) {
		return nil, ErrNotFound
	}

	if err != nil {
		return nil, err
	}

	return server, nil
}

func (s *Store) ResetDedicatedServerToken(ctx context.Context, serverID string, ownerPid uint32, tokenHash string) error {
	result, err := s.dedicatedServers().UpdateOne(ctx, bson.M{"serverId": serverID, "ownerPid": ownerPid},
		bson.M{"$set": bson.M{"tokenHash": tokenHash}})
	if err != nil {
		return err
	}

	if result.MatchedCount == 0 {
		return ErrNotFound
	}

	return nil
}

func (s *Store) DeleteDedicatedServer(ctx context.Context, serverID string, ownerPid uint32) error {
	result, err := s.dedicatedServers().DeleteOne(ctx, bson.M{"serverId": serverID, "ownerPid": ownerPid})
	if err != nil {
		return err
	}

	if result.DeletedCount == 0 {
		return ErrNotFound
	}

	return s.DeleteServerSessions(ctx, serverID)
}
