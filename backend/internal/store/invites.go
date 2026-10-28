package store

import (
	"context"
	"errors"
	"time"

	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"
)

type Invite struct {
	InviteID  string    `bson:"inviteId"  json:"inviteId"`
	SourcePid uint32    `bson:"sourcePid" json:"sourcePid"`
	TargetPid uint32    `bson:"targetPid" json:"targetPid"`
	Sid       uint64    `bson:"sid"       json:"sid,string"`
	CreatedAt time.Time `bson:"createdAt" json:"-"`
	ExpiresAt time.Time `bson:"expiresAt" json:"-"`
}

func (s *Store) CreateInvite(ctx context.Context, sourcePid uint32, targetPid uint32, sid uint64) (*Invite, error) {
	invite := &Invite{
		InviteID:  randomText(12),
		SourcePid: sourcePid,
		TargetPid: targetPid,
		Sid:       sid,
		CreatedAt: now(),
		ExpiresAt: now().Add(s.config.InviteTTL),
	}

	if _, err := s.invites().InsertOne(ctx, invite); err != nil {
		return nil, err
	}

	return invite, nil
}

func (s *Store) InviteByID(ctx context.Context, inviteID string) (*Invite, error) {
	invite := &Invite{}
	err := s.invites().FindOne(ctx, bson.M{"inviteId": inviteID}).Decode(invite)
	if errors.Is(err, mongo.ErrNoDocuments) {
		return nil, ErrNotFound
	}

	if err != nil {
		return nil, err
	}

	return invite, nil
}

func (s *Store) InvitesFor(ctx context.Context, pid uint32) (incoming []Invite, outgoing []Invite, err error) {
	cursor, err := s.invites().Find(ctx, bson.M{"$or": []bson.M{{"sourcePid": pid}, {"targetPid": pid}}})
	if err != nil {
		return nil, nil, err
	}

	defer cursor.Close(ctx)

	incoming = []Invite{}
	outgoing = []Invite{}
	for cursor.Next(ctx) {
		invite := Invite{}
		if err := cursor.Decode(&invite); err != nil {
			return nil, nil, err
		}

		if invite.TargetPid == pid {
			incoming = append(incoming, invite)
		} else {
			outgoing = append(outgoing, invite)
		}
	}

	return incoming, outgoing, cursor.Err()
}

func (s *Store) DeleteInvite(ctx context.Context, inviteID string) error {
	result, err := s.invites().DeleteOne(ctx, bson.M{"inviteId": inviteID})
	if err != nil {
		return err
	}

	if result.DeletedCount == 0 {
		return ErrNotFound
	}

	return nil
}
