package store

import (
	"context"
	"errors"
	"time"

	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

type FriendRequest struct {
	RequestID string    `bson:"requestId" json:"requestId"`
	SourcePid uint32    `bson:"sourcePid" json:"sourcePid"`
	TargetPid uint32    `bson:"targetPid" json:"targetPid"`
	CreatedAt time.Time `bson:"createdAt" json:"-"`
	ExpiresAt time.Time `bson:"expiresAt" json:"expiresAt"`
}

type Presence struct {
	Pid        uint32    `bson:"pid"        json:"pid"`
	State      string    `bson:"state"      json:"state"`
	Joinable   bool      `bson:"joinable"   json:"joinable"`
	CurrentSid uint64    `bson:"currentSid" json:"currentSid"`
	UpdatedAt  time.Time `bson:"updatedAt"  json:"-"`
	ExpiresAt  time.Time `bson:"expiresAt"  json:"-"`
}

func (s *Store) Friends(ctx context.Context, pid uint32) ([]uint32, error) {
	cursor, err := s.friendships().Find(ctx, bson.M{"$or": []bson.M{{"lowPid": pid}, {"highPid": pid}}})
	if err != nil {
		return nil, err
	}

	defer cursor.Close(ctx)

	friends := []uint32{}
	for cursor.Next(ctx) {
		var friendship struct {
			LowPid  uint32 `bson:"lowPid"`
			HighPid uint32 `bson:"highPid"`
		}

		if err := cursor.Decode(&friendship); err != nil {
			return nil, err
		}

		if friendship.LowPid == pid {
			friends = append(friends, friendship.HighPid)
		} else {
			friends = append(friends, friendship.LowPid)
		}
	}

	return friends, cursor.Err()
}

func (s *Store) AreFriends(ctx context.Context, first uint32, second uint32) (bool, error) {
	low, high := orderPids(first, second)
	count, err := s.friendships().CountDocuments(ctx, bson.M{"lowPid": low, "highPid": high})
	return count > 0, err
}

func (s *Store) AddFriendship(ctx context.Context, first uint32, second uint32) error {
	low, high := orderPids(first, second)
	_, err := s.friendships().UpdateOne(ctx,
		bson.M{"lowPid": low, "highPid": high},
		bson.M{"$setOnInsert": bson.M{"lowPid": low, "highPid": high, "createdAt": now()}},
		options.Update().SetUpsert(true))
	if mongo.IsDuplicateKeyError(err) {
		return nil
	}

	return err
}

func (s *Store) RemoveFriendship(ctx context.Context, first uint32, second uint32) error {
	low, high := orderPids(first, second)
	_, err := s.friendships().DeleteOne(ctx, bson.M{"lowPid": low, "highPid": high})
	return err
}

func (s *Store) CreateFriendRequest(ctx context.Context, sourcePid uint32, targetPid uint32) (*FriendRequest, error) {
	request := &FriendRequest{
		RequestID: randomText(12),
		SourcePid: sourcePid,
		TargetPid: targetPid,
		CreatedAt: now(),
		ExpiresAt: now().Add(s.config.RequestTTL),
	}

	if _, err := s.friendRequests().InsertOne(ctx, request); err != nil {
		if mongo.IsDuplicateKeyError(err) {
			return nil, ErrConflict
		}

		return nil, err
	}

	return request, nil
}

func (s *Store) FriendRequestByID(ctx context.Context, requestID string) (*FriendRequest, error) {
	request := &FriendRequest{}
	err := s.friendRequests().FindOne(ctx, bson.M{"requestId": requestID}).Decode(request)
	if errors.Is(err, mongo.ErrNoDocuments) {
		return nil, ErrNotFound
	}

	if err != nil {
		return nil, err
	}

	return request, nil
}

func (s *Store) FriendRequestsFor(ctx context.Context, pid uint32) (incoming []FriendRequest, outgoing []FriendRequest, err error) {
	cursor, err := s.friendRequests().Find(ctx, bson.M{"$or": []bson.M{{"sourcePid": pid}, {"targetPid": pid}}})
	if err != nil {
		return nil, nil, err
	}

	defer cursor.Close(ctx)

	incoming = []FriendRequest{}
	outgoing = []FriendRequest{}
	for cursor.Next(ctx) {
		request := FriendRequest{}
		if err := cursor.Decode(&request); err != nil {
			return nil, nil, err
		}

		if request.TargetPid == pid {
			incoming = append(incoming, request)
		} else {
			outgoing = append(outgoing, request)
		}
	}

	return incoming, outgoing, cursor.Err()
}

func (s *Store) DeleteFriendRequest(ctx context.Context, requestID string) error {
	result, err := s.friendRequests().DeleteOne(ctx, bson.M{"requestId": requestID})
	if err != nil {
		return err
	}

	if result.DeletedCount == 0 {
		return ErrNotFound
	}

	return nil
}

func (s *Store) SetPresence(ctx context.Context, presence Presence) (*Presence, error) {
	presence.UpdatedAt = now()
	presence.ExpiresAt = now().Add(s.config.PresenceTTL)

	_, err := s.presence().ReplaceOne(ctx, bson.M{"pid": presence.Pid}, presence, options.Replace().SetUpsert(true))
	if err != nil {
		return nil, err
	}

	return &presence, nil
}

func (s *Store) PresenceByPid(ctx context.Context, pid uint32) (*Presence, error) {
	presence := &Presence{}
	err := s.presence().FindOne(ctx, bson.M{"pid": pid}).Decode(presence)
	if errors.Is(err, mongo.ErrNoDocuments) {
		return nil, ErrNotFound
	}

	if err != nil {
		return nil, err
	}

	return presence, nil
}

func (s *Store) ClearPresence(ctx context.Context, pid uint32) error {
	_, err := s.presence().DeleteOne(ctx, bson.M{"pid": pid})
	return err
}

func orderPids(first uint32, second uint32) (low uint32, high uint32) {
	if first <= second {
		return first, second
	}

	return second, first
}
