package store

import (
	"context"
	"errors"

	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

const (
	SkinModelClassic = "classic"
	SkinModelSlim    = "slim"
)

type texture struct {
	Hash string `bson:"_id"`
	Data []byte `bson:"data"`
}

func (s *Store) SaveTexture(ctx context.Context, hash string, data []byte) error {
	_, err := s.textures().UpdateOne(ctx, bson.M{"_id": hash},
		bson.M{"$setOnInsert": texture{Hash: hash, Data: data}}, options.Update().SetUpsert(true))
	return err
}

func (s *Store) Texture(ctx context.Context, hash string) ([]byte, error) {
	found := texture{}
	err := s.textures().FindOne(ctx, bson.M{"_id": hash}).Decode(&found)
	if errors.Is(err, mongo.ErrNoDocuments) {
		return nil, ErrNotFound
	}

	if err != nil {
		return nil, err
	}

	return found.Data, nil
}

func (s *Store) SetAccountSkin(ctx context.Context, pid uint32, fields bson.M) error {
	fields["updatedAt"] = now()
	result, err := s.accounts().UpdateOne(ctx, bson.M{"pid": pid}, bson.M{"$set": fields})
	if err != nil {
		return err
	}

	if result.MatchedCount == 0 {
		return ErrNotFound
	}

	return nil
}
