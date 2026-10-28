package store

import (
	"context"
	"errors"
	"time"

	"github.com/jackc/pgx/v5"
)

type AuthSession struct {
	ID               string
	Pid              uint32
	InstallID        string
	Address          string
	CreatedAt        time.Time
	AccessExpiresAt  time.Time
	RefreshExpiresAt time.Time
}

const WebsiteInstallID = "website"

// CreateAuthSession revokes the account's other sessions of the same kind and stores a new one.
// Everstead allows one signed in game install per account, which is what lets the
// client report "this account was logged in somewhere else". Website and game sessions
// are separate so signing in on the website keeps the game signed in.
func (s *Store) CreateAuthSession(ctx context.Context, pid uint32, installID string, address string,
	accessHash string, refreshHash string) (*AuthSession, error) {
	if err := s.RevokeAccountSessions(ctx, pid, installID == WebsiteInstallID); err != nil {
		return nil, err
	}

	session := &AuthSession{
		ID:               randomText(16),
		Pid:              pid,
		InstallID:        installID,
		Address:          address,
		CreatedAt:        now(),
		AccessExpiresAt:  now().Add(s.config.AccessTokenTTL),
		RefreshExpiresAt: now().Add(s.config.RefreshTokenTTL),
	}

	_, err := s.postgres.Exec(ctx, `
		INSERT INTO auth_sessions (id, pid, access_token_hash, refresh_token_hash, install_id, address,
		                           created_at, access_expires_at, refresh_expires_at)
		VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9)`,
		session.ID, int64(pid), accessHash, refreshHash, installID, address,
		session.CreatedAt, session.AccessExpiresAt, session.RefreshExpiresAt)
	if err != nil {
		return nil, err
	}

	return session, nil
}

func (s *Store) AuthSessionByAccessHash(ctx context.Context, accessHash string) (*AuthSession, error) {
	return s.findAuthSession(ctx, `
		SELECT id, pid, install_id, address, created_at, access_expires_at, refresh_expires_at
		FROM auth_sessions
		WHERE access_token_hash = $1 AND revoked_at IS NULL AND access_expires_at > now()`, accessHash)
}

func (s *Store) AuthSessionByRefreshHash(ctx context.Context, refreshHash string) (*AuthSession, error) {
	return s.findAuthSession(ctx, `
		SELECT id, pid, install_id, address, created_at, access_expires_at, refresh_expires_at
		FROM auth_sessions
		WHERE refresh_token_hash = $1 AND revoked_at IS NULL AND refresh_expires_at > now()`, refreshHash)
}

func (s *Store) RotateAuthSession(ctx context.Context, id string, accessHash string, refreshHash string) error {
	_, err := s.postgres.Exec(ctx, `
		UPDATE auth_sessions
		SET access_token_hash = $2, refresh_token_hash = $3, access_expires_at = $4, refresh_expires_at = $5
		WHERE id = $1`,
		id, accessHash, refreshHash, now().Add(s.config.AccessTokenTTL), now().Add(s.config.RefreshTokenTTL))
	return err
}

func (s *Store) RevokeAuthSession(ctx context.Context, id string) error {
	_, err := s.postgres.Exec(ctx, `UPDATE auth_sessions SET revoked_at = now() WHERE id = $1 AND revoked_at IS NULL`, id)
	return err
}

func (s *Store) RevokeAccountSessions(ctx context.Context, pid uint32, website bool) error {
	_, err := s.postgres.Exec(ctx, `
		UPDATE auth_sessions SET revoked_at = now()
		WHERE pid = $1 AND revoked_at IS NULL AND (install_id = $2) = $3`, int64(pid), WebsiteInstallID, website)
	return err
}

func (s *Store) AllowRequest(ctx context.Context, bucket string, window time.Duration, limit int) (bool, error) {
	var hits int
	err := s.postgres.QueryRow(ctx, `
		INSERT INTO rate_limits (bucket, window_start, hits)
		VALUES ($1, now(), 1)
		ON CONFLICT (bucket) DO UPDATE
		SET window_start = CASE WHEN rate_limits.window_start < now() - $2::interval THEN now() ELSE rate_limits.window_start END,
		    hits        = CASE WHEN rate_limits.window_start < now() - $2::interval THEN 1 ELSE rate_limits.hits + 1 END
		RETURNING hits`, bucket, window.String()).Scan(&hits)
	if err != nil {
		return false, err
	}

	return hits <= limit, nil
}

func (s *Store) RecordLogin(ctx context.Context, pid uint32, username string, address string, succeeded bool, code string) {
	_, _ = s.postgres.Exec(ctx, `
		INSERT INTO login_audit (pid, username, address, succeeded, code)
		VALUES ($1, $2, $3, $4, $5)`, int64(pid), username, address, succeeded, code)
}

func (s *Store) findAuthSession(ctx context.Context, query string, argument string) (*AuthSession, error) {
	session := &AuthSession{}
	var pid int64
	err := s.postgres.QueryRow(ctx, query, argument).Scan(&session.ID, &pid, &session.InstallID,
		&session.Address, &session.CreatedAt, &session.AccessExpiresAt, &session.RefreshExpiresAt)
	if errors.Is(err, pgx.ErrNoRows) {
		return nil, ErrNotFound
	}

	if err != nil {
		return nil, err
	}

	session.Pid = uint32(pid)
	return session, nil
}
