package store

import (
	"context"
	"errors"
	"fmt"
	"log"
	"regexp"
	"strconv"
	"strings"
	"time"

	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

const adminUsername = "Admin"

type LoginRecord struct {
	Address   string
	Succeeded bool
	Code      string
	CreatedAt time.Time
}

func (s *Store) ensureAdminAccount(ctx context.Context) error {
	existing, err := s.AccountByUsername(ctx, adminUsername)
	if err != nil && !errors.Is(err, ErrNotFound) {
		return err
	}

	// A player who registered the name before the built in account existed gets renamed out of the way
	if existing != nil && !existing.BuiltIn {
		renamed := "admin_" + strconv.FormatUint(uint64(existing.Pid), 10)
		log.Printf("store: renaming account %d from %s to %s", existing.Pid, existing.Username, renamed)
		if err := s.SetAccountFields(ctx, existing.Pid, bson.M{"username": renamed, "usernameLower": renamed}); err != nil {
			return err
		}

		existing = nil
	}

	if existing == nil {
		// The hash only matters when the account is first created, after that the password is changed on the site
		if s.config.AdminPasswordHash == "" {
			return errors.New("EVERSTEAD_ADMIN_PASSWORD_HASH is required to create the admin account")
		}

		created, err := s.CreateAccount(ctx, adminUsername, "admin@everstead.invalid", s.config.AdminPasswordHash)
		if err != nil {
			return err
		}

		existing = created
	}

	return s.SetAccountFields(ctx, existing.Pid, bson.M{"builtIn": true, "permission": RoleAdmin, "disabled": false})
}

func (s *Store) SetAccountFields(ctx context.Context, pid uint32, fields bson.M) error {
	fields["updatedAt"] = now()
	result, err := s.accounts().UpdateOne(ctx, bson.M{"pid": pid}, bson.M{"$set": fields})
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

func (s *Store) SearchAccounts(ctx context.Context, query string, limit int64) ([]Account, error) {
	filter := bson.M{}
	query = strings.ToLower(strings.TrimSpace(query))
	if query != "" {
		pattern := regexp.QuoteMeta(query)
		or := []bson.M{{"usernameLower": bson.M{"$regex": "^" + pattern}}, {"emailLower": bson.M{"$regex": pattern}}}
		if pid, err := strconv.ParseUint(query, 10, 32); err == nil {
			or = append(or, bson.M{"pid": uint32(pid)})
		}

		filter["$or"] = or
	}

	accounts := []Account{}
	cursor, err := s.accounts().Find(ctx, filter, options.Find().SetSort(bson.D{{Key: "createdAt", Value: -1}}).SetLimit(limit))
	if err != nil {
		return nil, err
	}

	defer cursor.Close(ctx)

	if err := cursor.All(ctx, &accounts); err != nil {
		return nil, err
	}

	return accounts, nil
}

func (s *Store) CountAccounts(ctx context.Context) (int64, error) {
	return s.accounts().CountDocuments(ctx, bson.M{})
}

func (s *Store) RevokeAllAccountSessions(ctx context.Context, pid uint32) error {
	_, err := s.postgres.Exec(ctx, `UPDATE auth_sessions SET revoked_at = now() WHERE pid = $1 AND revoked_at IS NULL`, int64(pid))
	return err
}

func (s *Store) ActiveAuthSessions(ctx context.Context, pid uint32) ([]AuthSession, error) {
	rows, err := s.postgres.Query(ctx, `
		SELECT id, install_id, address, created_at, access_expires_at, refresh_expires_at
		FROM auth_sessions
		WHERE pid = $1 AND revoked_at IS NULL AND refresh_expires_at > now()
		ORDER BY created_at DESC`, int64(pid))
	if err != nil {
		return nil, err
	}

	defer rows.Close()

	sessions := []AuthSession{}
	for rows.Next() {
		session := AuthSession{Pid: pid}
		if err := rows.Scan(&session.ID, &session.InstallID, &session.Address, &session.CreatedAt,
			&session.AccessExpiresAt, &session.RefreshExpiresAt); err != nil {
			return nil, err
		}

		sessions = append(sessions, session)
	}

	return sessions, rows.Err()
}

func (s *Store) LoginHistory(ctx context.Context, pid uint32, limit int) ([]LoginRecord, error) {
	rows, err := s.postgres.Query(ctx, `
		SELECT address, succeeded, code, created_at FROM login_audit
		WHERE pid = $1 ORDER BY created_at DESC LIMIT $2`, int64(pid), limit)
	if err != nil {
		return nil, err
	}

	defer rows.Close()

	records := []LoginRecord{}
	for rows.Next() {
		record := LoginRecord{}
		if err := rows.Scan(&record.Address, &record.Succeeded, &record.Code, &record.CreatedAt); err != nil {
			return nil, err
		}

		records = append(records, record)
	}

	return records, rows.Err()
}

// DeleteAccount removes the account and everything tied to it, but keeps its forum posts so threads stay readable
func (s *Store) DeleteAccount(ctx context.Context, pid uint32) error {
	if err := s.RevokeAllAccountSessions(ctx, pid); err != nil {
		return err
	}

	if err := s.DeleteHostSessions(ctx, pid); err != nil {
		return err
	}

	deletions := []struct {
		collection *mongo.Collection
		filter     bson.M
	}{
		{s.dedicatedServers(), bson.M{"ownerPid": pid}},
		{s.friendships(), bson.M{"$or": []bson.M{{"lowPid": pid}, {"highPid": pid}}}},
		{s.friendRequests(), bson.M{"$or": []bson.M{{"sourcePid": pid}, {"targetPid": pid}}}},
		{s.invites(), bson.M{"$or": []bson.M{{"sourcePid": pid}, {"targetPid": pid}}}},
		{s.presence(), bson.M{"pid": pid}},
		{s.forumVotes(), bson.M{"pid": pid}},
	}

	for _, deletion := range deletions {
		if _, err := deletion.collection.DeleteMany(ctx, deletion.filter); err != nil {
			return fmt.Errorf("delete from %s: %w", deletion.collection.Name(), err)
		}
	}

	result, err := s.accounts().DeleteOne(ctx, bson.M{"pid": pid})
	if err != nil {
		return err
	}

	if result.DeletedCount == 0 {
		return ErrNotFound
	}

	return nil
}
