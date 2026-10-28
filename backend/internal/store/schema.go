package store

import (
	"context"
	"fmt"

	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

const postgresSchema = `
CREATE TABLE IF NOT EXISTS auth_sessions (
    id                 TEXT PRIMARY KEY,
    pid                BIGINT      NOT NULL,
    access_token_hash  TEXT        NOT NULL UNIQUE,
    refresh_token_hash TEXT        NOT NULL UNIQUE,
    install_id         TEXT        NOT NULL DEFAULT '',
    address            TEXT        NOT NULL DEFAULT '',
    created_at         TIMESTAMPTZ NOT NULL DEFAULT now(),
    access_expires_at  TIMESTAMPTZ NOT NULL,
    refresh_expires_at TIMESTAMPTZ NOT NULL,
    revoked_at         TIMESTAMPTZ
);

CREATE INDEX IF NOT EXISTS auth_sessions_pid_idx ON auth_sessions (pid);
CREATE INDEX IF NOT EXISTS auth_sessions_refresh_expiry_idx ON auth_sessions (refresh_expires_at);

CREATE TABLE IF NOT EXISTS rate_limits (
    bucket       TEXT PRIMARY KEY,
    window_start TIMESTAMPTZ NOT NULL,
    hits         INTEGER     NOT NULL
);

CREATE TABLE IF NOT EXISTS login_audit (
    id         BIGSERIAL PRIMARY KEY,
    pid        BIGINT      NOT NULL DEFAULT 0,
    username   TEXT        NOT NULL DEFAULT '',
    address    TEXT        NOT NULL DEFAULT '',
    succeeded  BOOLEAN     NOT NULL,
    code       TEXT        NOT NULL DEFAULT '',
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS login_audit_pid_idx ON login_audit (pid, created_at DESC);
`

func (s *Store) prepare(ctx context.Context) error {
	if _, err := s.postgres.Exec(ctx, postgresSchema); err != nil {
		return fmt.Errorf("prepare postgres: %w", err)
	}

	indexes := []struct {
		collection *mongo.Collection
		model      mongo.IndexModel
	}{
		{s.accounts(), mongo.IndexModel{Keys: bson.D{{Key: "pid", Value: 1}}, Options: options.Index().SetUnique(true)}},
		{s.accounts(), mongo.IndexModel{Keys: bson.D{{Key: "usernameLower", Value: 1}}, Options: options.Index().SetUnique(true)}},
		{s.accounts(), mongo.IndexModel{Keys: bson.D{{Key: "emailLower", Value: 1}}, Options: options.Index().SetUnique(true)}},

		{s.friendships(), mongo.IndexModel{Keys: bson.D{{Key: "lowPid", Value: 1}, {Key: "highPid", Value: 1}}, Options: options.Index().SetUnique(true)}},
		{s.friendships(), mongo.IndexModel{Keys: bson.D{{Key: "highPid", Value: 1}}}},

		{s.friendRequests(), mongo.IndexModel{Keys: bson.D{{Key: "requestId", Value: 1}}, Options: options.Index().SetUnique(true)}},
		{s.friendRequests(), mongo.IndexModel{Keys: bson.D{{Key: "sourcePid", Value: 1}, {Key: "targetPid", Value: 1}}, Options: options.Index().SetUnique(true)}},
		{s.friendRequests(), mongo.IndexModel{Keys: bson.D{{Key: "targetPid", Value: 1}}}},
		{s.friendRequests(), mongo.IndexModel{Keys: bson.D{{Key: "expiresAt", Value: 1}}, Options: options.Index().SetExpireAfterSeconds(0)}},

		{s.presence(), mongo.IndexModel{Keys: bson.D{{Key: "pid", Value: 1}}, Options: options.Index().SetUnique(true)}},
		{s.presence(), mongo.IndexModel{Keys: bson.D{{Key: "expiresAt", Value: 1}}, Options: options.Index().SetExpireAfterSeconds(0)}},

		{s.invites(), mongo.IndexModel{Keys: bson.D{{Key: "inviteId", Value: 1}}, Options: options.Index().SetUnique(true)}},
		{s.invites(), mongo.IndexModel{Keys: bson.D{{Key: "targetPid", Value: 1}}}},
		{s.invites(), mongo.IndexModel{Keys: bson.D{{Key: "sourcePid", Value: 1}}}},
		{s.invites(), mongo.IndexModel{Keys: bson.D{{Key: "expiresAt", Value: 1}}, Options: options.Index().SetExpireAfterSeconds(0)}},

		{s.gameSessions(), mongo.IndexModel{Keys: bson.D{{Key: "sid", Value: 1}}, Options: options.Index().SetUnique(true)}},
		{s.gameSessions(), mongo.IndexModel{Keys: bson.D{{Key: "hostPid", Value: 1}}}},
		{s.gameSessions(), mongo.IndexModel{Keys: bson.D{{Key: "hostKey", Value: 1}}}},
		{s.gameSessions(), mongo.IndexModel{Keys: bson.D{{Key: "serverId", Value: 1}}}},
		{s.gameSessions(), mongo.IndexModel{Keys: bson.D{{Key: "expiresAt", Value: 1}}, Options: options.Index().SetExpireAfterSeconds(0)}},

		{s.sessionPeers(), mongo.IndexModel{Keys: bson.D{{Key: "sid", Value: 1}, {Key: "pid", Value: 1}}, Options: options.Index().SetUnique(true)}},
		{s.sessionPeers(), mongo.IndexModel{Keys: bson.D{{Key: "expiresAt", Value: 1}}, Options: options.Index().SetExpireAfterSeconds(0)}},

		{s.joinTickets(), mongo.IndexModel{Keys: bson.D{{Key: "ticket", Value: 1}}, Options: options.Index().SetUnique(true)}},
		{s.joinTickets(), mongo.IndexModel{Keys: bson.D{{Key: "expiresAt", Value: 1}}, Options: options.Index().SetExpireAfterSeconds(0)}},

		{s.dedicatedServers(), mongo.IndexModel{Keys: bson.D{{Key: "serverId", Value: 1}}, Options: options.Index().SetUnique(true)}},
		{s.dedicatedServers(), mongo.IndexModel{Keys: bson.D{{Key: "nameLower", Value: 1}}, Options: options.Index().SetUnique(true)}},
		{s.dedicatedServers(), mongo.IndexModel{Keys: bson.D{{Key: "tokenHash", Value: 1}}, Options: options.Index().SetUnique(true)}},
		{s.dedicatedServers(), mongo.IndexModel{Keys: bson.D{{Key: "ownerPid", Value: 1}}}},

		{s.forumThreads(), mongo.IndexModel{Keys: bson.D{{Key: "threadId", Value: 1}}, Options: options.Index().SetUnique(true)}},
		{s.forumThreads(), mongo.IndexModel{Keys: bson.D{{Key: "category", Value: 1}, {Key: "pinned", Value: -1}, {Key: "lastPostAt", Value: -1}}}},
		{s.forumThreads(), mongo.IndexModel{Keys: bson.D{{Key: "authorPid", Value: 1}, {Key: "createdAt", Value: -1}}}},

		{s.forumPosts(), mongo.IndexModel{Keys: bson.D{{Key: "postId", Value: 1}}, Options: options.Index().SetUnique(true)}},
		{s.forumPosts(), mongo.IndexModel{Keys: bson.D{{Key: "threadId", Value: 1}, {Key: "createdAt", Value: 1}}}},
		{s.forumPosts(), mongo.IndexModel{Keys: bson.D{{Key: "authorPid", Value: 1}}}},

		{s.forumVotes(), mongo.IndexModel{Keys: bson.D{{Key: "threadId", Value: 1}, {Key: "pid", Value: 1}}, Options: options.Index().SetUnique(true)}},

		{s.forumReports(), mongo.IndexModel{Keys: bson.D{{Key: "reportId", Value: 1}}, Options: options.Index().SetUnique(true)}},
		{s.forumReports(), mongo.IndexModel{Keys: bson.D{{Key: "resolved", Value: 1}, {Key: "createdAt", Value: 1}}}},

		{s.modLog(), mongo.IndexModel{Keys: bson.D{{Key: "createdAt", Value: -1}}}},
		{s.modLog(), mongo.IndexModel{Keys: bson.D{{Key: "targetPid", Value: 1}, {Key: "createdAt", Value: -1}}}},
	}

	for _, index := range indexes {
		if _, err := index.collection.Indexes().CreateOne(ctx, index.model); err != nil {
			return fmt.Errorf("prepare mongo index on %s: %w", index.collection.Name(), err)
		}
	}

	return nil
}
