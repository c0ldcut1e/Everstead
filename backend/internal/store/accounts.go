package store

import (
	"context"
	"errors"
	"strings"
	"time"

	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

var (
	ErrNotFound              = errors.New("store: not found")
	ErrConflict              = errors.New("store: already exists")
	ErrUsernameChangeTooSoon = errors.New("store: username cannot be changed yet")
)

const firstPid = 1000000

const (
	RoleUser      = 0
	RoleModerator = 1
	RoleAdmin     = 2
)

type Account struct {
	Pid               uint32    `bson:"pid"              json:"pid"`
	Version           int       `bson:"version"          json:"version"`
	Username          string    `bson:"username"         json:"username"`
	UsernameLower     string    `bson:"usernameLower"    json:"-"`
	Email             string    `bson:"email"            json:"email"`
	EmailLower        string    `bson:"emailLower"       json:"-"`
	PasswordHash      string    `bson:"passwordHash"     json:"-"`
	UsernameChangedAt time.Time `bson:"usernameChangedAt" json:"-"`
	Permission        int       `bson:"permission"       json:"permission"`
	Disabled          bool      `bson:"disabled"         json:"disabled"`
	BuiltIn           bool      `bson:"builtIn"          json:"-"`
	ForumTitle        string    `bson:"forumTitle"       json:"-"`
	MutedUntil        time.Time `bson:"mutedUntil"       json:"-"`
	MuteReason        string    `bson:"muteReason"       json:"-"`
	SkinHash          string    `bson:"skinHash"         json:"skinHash"`
	SkinModel         string    `bson:"skinModel"        json:"skinModel"`
	Cape              string    `bson:"cape"             json:"cape"`
	Capes             []string  `bson:"capes"            json:"capes"`
	LinkedInstallIds  []string  `bson:"linkedInstallIds" json:"linkedInstallIds"`
	LinkedAddresses   []string  `bson:"linkedAddresses"  json:"linkedAddresses"`
	CreatedAt         time.Time `bson:"createdAt"        json:"-"`
	UpdatedAt         time.Time `bson:"updatedAt"        json:"-"`
}

func (s *Store) CreateAccount(ctx context.Context, username string, email string, passwordHash string) (*Account, error) {
	usernameLower := strings.ToLower(username)
	emailLower := strings.ToLower(email)

	taken, err := s.accounts().CountDocuments(ctx, bson.M{"$or": []bson.M{
		{"usernameLower": usernameLower},
		{"emailLower": emailLower},
	}})
	if err != nil {
		return nil, err
	}

	if taken > 0 {
		return nil, ErrConflict
	}

	pid, err := s.nextNumber(ctx, "pid", firstPid)
	if err != nil {
		return nil, err
	}

	account := &Account{
		Pid:              uint32(pid),
		Version:          1,
		Username:         username,
		UsernameLower:    usernameLower,
		Email:            email,
		EmailLower:       emailLower,
		PasswordHash:     passwordHash,
		LinkedInstallIds: []string{},
		LinkedAddresses:  []string{},
		CreatedAt:        now(),
		UpdatedAt:        now(),
	}

	if _, err := s.accounts().InsertOne(ctx, account); err != nil {
		if mongo.IsDuplicateKeyError(err) {
			return nil, ErrConflict
		}

		return nil, err
	}

	return account, nil
}

func (s *Store) AccountByPid(ctx context.Context, pid uint32) (*Account, error) {
	return s.findAccount(ctx, bson.M{"pid": pid})
}

func (s *Store) AccountByUsername(ctx context.Context, username string) (*Account, error) {
	return s.findAccount(ctx, bson.M{"usernameLower": strings.ToLower(username)})
}

func (s *Store) AccountByEmail(ctx context.Context, email string) (*Account, error) {
	return s.findAccount(ctx, bson.M{"emailLower": strings.ToLower(email)})
}

func (s *Store) AccountsByPids(ctx context.Context, pids []uint32) (map[uint32]*Account, error) {
	found := map[uint32]*Account{}
	if len(pids) == 0 {
		return found, nil
	}

	cursor, err := s.accounts().Find(ctx, bson.M{"pid": bson.M{"$in": pids}})
	if err != nil {
		return nil, err
	}

	defer cursor.Close(ctx)

	for cursor.Next(ctx) {
		account := &Account{}
		if err := cursor.Decode(account); err != nil {
			return nil, err
		}

		found[account.Pid] = account
	}

	return found, cursor.Err()
}

func (s *Store) LinkAccountIdentity(ctx context.Context, pid uint32, installID string, address string) error {
	update := bson.M{"$set": bson.M{"updatedAt": now()}}
	linked := bson.M{}
	if installID != "" {
		linked["linkedInstallIds"] = installID
	}

	if address != "" {
		linked["linkedAddresses"] = address
	}

	if len(linked) > 0 {
		update["$addToSet"] = linked
	}

	result, err := s.accounts().UpdateOne(ctx, bson.M{"pid": pid}, update)
	if err != nil {
		return err
	}

	if result.MatchedCount == 0 {
		return ErrNotFound
	}

	return nil
}

func (s *Store) SetAccountPassword(ctx context.Context, pid uint32, passwordHash string) error {
	result, err := s.accounts().UpdateOne(ctx, bson.M{"pid": pid},
		bson.M{"$set": bson.M{"passwordHash": passwordHash, "updatedAt": now()}})
	if err != nil {
		return err
	}

	if result.MatchedCount == 0 {
		return ErrNotFound
	}

	return nil
}

func (s *Store) SetAccountEmail(ctx context.Context, pid uint32, email string) error {
	result, err := s.accounts().UpdateOne(ctx, bson.M{"pid": pid},
		bson.M{"$set": bson.M{"email": email, "emailLower": strings.ToLower(email), "updatedAt": now()}})
	if mongo.IsDuplicateKeyError(err) {
		return ErrConflict
	}

	if err != nil {
		return err
	}

	if result.MatchedCount == 0 {
		return ErrNotFound
	}

	return nil
}

func (s *Store) ChangeAccountUsername(ctx context.Context, pid uint32, username string) error {
	usernameLower := strings.ToLower(username)
	usernameChangeCutoff := now().AddDate(0, 0, -30)
	result, err := s.accounts().UpdateOne(ctx, bson.M{
		"pid": pid,
		"$or": []bson.M{
			{"usernameChangedAt": bson.M{"$exists": false}},
			{"usernameChangedAt": bson.M{"$lte": usernameChangeCutoff}},
		},
	}, bson.M{"$set": bson.M{
		"username":          username,
		"usernameLower":     usernameLower,
		"usernameChangedAt": now(),
		"updatedAt":         now(),
	}})
	if mongo.IsDuplicateKeyError(err) {
		return ErrConflict
	}

	if err != nil {
		return err
	}

	if result.MatchedCount == 0 {
		return ErrUsernameChangeTooSoon
	}

	return nil
}

func (s *Store) findAccount(ctx context.Context, filter bson.M) (*Account, error) {
	account := &Account{}
	err := s.accounts().FindOne(ctx, filter, options.FindOne()).Decode(account)
	if errors.Is(err, mongo.ErrNoDocuments) {
		return nil, ErrNotFound
	}

	if err != nil {
		return nil, err
	}

	return account, nil
}
