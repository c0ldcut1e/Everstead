package store

import (
	"context"
	"errors"
	"time"

	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

type GameSession struct {
	Sid              uint64    `bson:"sid"            json:"sid,string"`
	HostPid          uint32    `bson:"hostPid"        json:"hostPid"`
	HostUsername     string    `bson:"hostUsername"   json:"hostUsername"`
	Name             string    `bson:"name"           json:"name"`
	Visibility       string    `bson:"visibility"     json:"visibility"`
	Joinable         bool      `bson:"joinable"       json:"joinable"`
	FriendsOfFriends bool      `bson:"friendsOfFriends" json:"friendsOfFriends"`
	MaxPlayers       int       `bson:"maxPlayers"     json:"maxPlayers"`
	CurrentPlayers   int       `bson:"currentPlayers" json:"currentPlayers"`
	ServerID         string    `bson:"serverId,omitempty" json:"-"`
	HostKey          string    `bson:"hostKey"        json:"-"`
	UpdatedAt        time.Time `bson:"updatedAt"      json:"-"`
	ExpiresAt        time.Time `bson:"expiresAt"      json:"-"`
}

type SessionPeer struct {
	Sid       uint64    `bson:"sid"       json:"sid,string"`
	Pid       uint32    `bson:"pid"       json:"pid"`
	Username  string    `bson:"username"  json:"username"`
	State     string    `bson:"state"     json:"state"`
	UpdatedAt time.Time `bson:"updatedAt" json:"-"`
	ExpiresAt time.Time `bson:"expiresAt" json:"-"`
}

type JoinTicket struct {
	Ticket    string    `bson:"ticket"    json:"ticket"`
	Sid       uint64    `bson:"sid"       json:"sid,string"`
	SourcePid uint32    `bson:"sourcePid" json:"sourcePid"`
	HostPid   uint32    `bson:"hostPid"   json:"hostPid"`
	HostKey   string    `bson:"hostKey"   json:"-"`
	CreatedAt time.Time `bson:"createdAt" json:"-"`
	ExpiresAt time.Time `bson:"expiresAt" json:"-"`
}

func (s *Store) CreateGameSession(ctx context.Context, session GameSession) (*GameSession, error) {
	var err error
	if session.ServerID != "" {
		err = s.DeleteServerSessions(ctx, session.ServerID)
	} else {
		err = s.DeleteHostSessions(ctx, session.HostPid)
	}

	if err != nil {
		return nil, err
	}

	session.Sid = randomNumber()
	session.HostKey = randomText(16)
	session.UpdatedAt = now()
	session.ExpiresAt = now().Add(s.config.SessionTTL)

	if _, err := s.gameSessions().InsertOne(ctx, session); err != nil {
		return nil, err
	}

	return &session, nil
}

// sessionOwnerFilter matches a session by its server when serverID is set and by its host player otherwise
func sessionOwnerFilter(sid uint64, hostPid uint32, serverID string) bson.M {
	if serverID != "" {
		return bson.M{"sid": sid, "serverId": serverID}
	}

	return bson.M{"sid": sid, "hostPid": hostPid}
}

func (s *Store) UpdateGameSession(ctx context.Context, sid uint64, hostPid uint32, serverID string, session GameSession) (*GameSession, error) {
	update := bson.M{"$set": bson.M{
		"name":             session.Name,
		"visibility":       session.Visibility,
		"joinable":         session.Joinable,
		"friendsOfFriends": session.FriendsOfFriends,
		"maxPlayers":       session.MaxPlayers,
		"currentPlayers":   session.CurrentPlayers,
		"hostUsername":     session.HostUsername,
		"updatedAt":        now(),
		"expiresAt":        now().Add(s.config.SessionTTL),
	}}

	updated := &GameSession{}
	err := s.gameSessions().FindOneAndUpdate(ctx, sessionOwnerFilter(sid, hostPid, serverID), update,
		options.FindOneAndUpdate().SetReturnDocument(options.After)).Decode(updated)
	if errors.Is(err, mongo.ErrNoDocuments) {
		return nil, ErrNotFound
	}

	if err != nil {
		return nil, err
	}

	return updated, nil
}

// DeleteHostSessions drops every session a player is hosting, so one account can
// never advertise two worlds at once.
func (s *Store) DeleteHostSessions(ctx context.Context, hostPid uint32) error {
	return s.deleteSessions(ctx, bson.M{"hostPid": hostPid, "serverId": bson.M{"$exists": false}})
}

func (s *Store) DeleteServerSessions(ctx context.Context, serverID string) error {
	return s.deleteSessions(ctx, bson.M{"serverId": serverID})
}

func (s *Store) GameSessionByServer(ctx context.Context, serverID string) (*GameSession, error) {
	session := &GameSession{}
	err := s.gameSessions().FindOne(ctx, bson.M{"serverId": serverID}).Decode(session)
	if errors.Is(err, mongo.ErrNoDocuments) {
		return nil, ErrNotFound
	}

	if err != nil {
		return nil, err
	}

	return session, nil
}

func (s *Store) deleteSessions(ctx context.Context, filter bson.M) error {
	cursor, err := s.gameSessions().Find(ctx, filter)
	if err != nil {
		return err
	}

	defer cursor.Close(ctx)

	stale := []uint64{}
	for cursor.Next(ctx) {
		var session struct {
			Sid uint64 `bson:"sid"`
		}

		if err := cursor.Decode(&session); err != nil {
			return err
		}

		stale = append(stale, session.Sid)
	}

	if err := cursor.Err(); err != nil {
		return err
	}

	if len(stale) == 0 {
		return nil
	}

	if _, err := s.gameSessions().DeleteMany(ctx, bson.M{"sid": bson.M{"$in": stale}}); err != nil {
		return err
	}

	_, _ = s.sessionPeers().DeleteMany(ctx, bson.M{"sid": bson.M{"$in": stale}})
	_, _ = s.joinTickets().DeleteMany(ctx, bson.M{"sid": bson.M{"$in": stale}})
	return nil
}

func (s *Store) GameSessionBySid(ctx context.Context, sid uint64) (*GameSession, error) {
	session := &GameSession{}
	err := s.gameSessions().FindOne(ctx, bson.M{"sid": sid}).Decode(session)
	if errors.Is(err, mongo.ErrNoDocuments) {
		return nil, ErrNotFound
	}

	if err != nil {
		return nil, err
	}

	return session, nil
}

func (s *Store) GameSessionByHostKey(ctx context.Context, hostKey string) (*GameSession, error) {
	session := &GameSession{}
	err := s.gameSessions().FindOne(ctx, bson.M{"hostKey": hostKey}).Decode(session)
	if errors.Is(err, mongo.ErrNoDocuments) {
		return nil, ErrNotFound
	}

	if err != nil {
		return nil, err
	}

	return session, nil
}

func (s *Store) GameSessionsForHosts(ctx context.Context, hostPids []uint32) ([]GameSession, error) {
	sessions := []GameSession{}
	if len(hostPids) == 0 {
		return sessions, nil
	}

	cursor, err := s.gameSessions().Find(ctx, bson.M{"hostPid": bson.M{"$in": hostPids}})
	if err != nil {
		return nil, err
	}

	defer cursor.Close(ctx)

	if err := cursor.All(ctx, &sessions); err != nil {
		return nil, err
	}

	return sessions, nil
}

func (s *Store) GameSessionsWithPeers(ctx context.Context, pids []uint32) ([]GameSession, error) {
	sessions := []GameSession{}
	if len(pids) == 0 {
		return sessions, nil
	}

	sids, err := s.sessionPeers().Distinct(ctx, "sid", bson.M{"pid": bson.M{"$in": pids}})
	if err != nil {
		return nil, err
	}

	if len(sids) == 0 {
		return sessions, nil
	}

	cursor, err := s.gameSessions().Find(ctx, bson.M{"sid": bson.M{"$in": sids}})
	if err != nil {
		return nil, err
	}

	defer cursor.Close(ctx)

	if err := cursor.All(ctx, &sessions); err != nil {
		return nil, err
	}

	return sessions, nil
}

func (s *Store) DeleteGameSession(ctx context.Context, sid uint64, hostPid uint32, serverID string) error {
	result, err := s.gameSessions().DeleteOne(ctx, sessionOwnerFilter(sid, hostPid, serverID))
	if err != nil {
		return err
	}

	if result.DeletedCount == 0 {
		return ErrNotFound
	}

	_, _ = s.sessionPeers().DeleteMany(ctx, bson.M{"sid": sid})
	_, _ = s.joinTickets().DeleteMany(ctx, bson.M{"sid": sid})
	return nil
}

func (s *Store) UpsertSessionPeer(ctx context.Context, peer SessionPeer) (*SessionPeer, error) {
	peer.UpdatedAt = now()
	peer.ExpiresAt = now().Add(s.config.SessionTTL)

	_, err := s.sessionPeers().ReplaceOne(ctx, bson.M{"sid": peer.Sid, "pid": peer.Pid}, peer,
		options.Replace().SetUpsert(true))
	if err != nil {
		return nil, err
	}

	return &peer, nil
}

func (s *Store) SessionPeers(ctx context.Context, sid uint64) ([]SessionPeer, error) {
	peers := []SessionPeer{}
	cursor, err := s.sessionPeers().Find(ctx, bson.M{"sid": sid})
	if err != nil {
		return nil, err
	}

	defer cursor.Close(ctx)

	if err := cursor.All(ctx, &peers); err != nil {
		return nil, err
	}

	return peers, nil
}

func (s *Store) DeleteSessionPeer(ctx context.Context, sid uint64, pid uint32) error {
	_, err := s.sessionPeers().DeleteOne(ctx, bson.M{"sid": sid, "pid": pid})
	return err
}

func (s *Store) CreateJoinTicket(ctx context.Context, session *GameSession, sourcePid uint32) (*JoinTicket, error) {
	ticket := &JoinTicket{
		Ticket:    randomText(16),
		Sid:       session.Sid,
		SourcePid: sourcePid,
		HostPid:   session.HostPid,
		HostKey:   session.HostKey,
		CreatedAt: now(),
		ExpiresAt: now().Add(s.config.InviteTTL),
	}

	if _, err := s.joinTickets().InsertOne(ctx, ticket); err != nil {
		return nil, err
	}

	return ticket, nil
}

func (s *Store) JoinTicketByText(ctx context.Context, text string) (*JoinTicket, error) {
	ticket := &JoinTicket{}
	err := s.joinTickets().FindOne(ctx, bson.M{"ticket": text}).Decode(ticket)
	if errors.Is(err, mongo.ErrNoDocuments) {
		return nil, ErrNotFound
	}

	if err != nil {
		return nil, err
	}

	return ticket, nil
}
