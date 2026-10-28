# Everstead Backend

The Go backend behind Everstead's online features. A single binary serves:

- the account API used by the game and the launcher;
- the account website for registering, logging in, and managing skins, capes and servers;
- the UDP rendezvous that introduces players to hosts for peer to peer connections.

Accounts, friends and textures are stored in MongoDB. Login sessions, rate limits and the login audit log are stored in PostgreSQL. The schema is created on startup.

## Running

From the repository root:

```sh
docker compose up --build
```

This starts the backend together with MongoDB, PostgreSQL, mongo-express and Adminer.

| Service | Address |
| --- | --- |
| API and website | `http://localhost:8088` |
| Rendezvous | UDP port `37373` |
| mongo-express | `http://127.0.0.1:8181` |
| Adminer | `http://127.0.0.1:8182` |

The databases and their web front ends only listen on `127.0.0.1`. The default credentials in `docker-compose.yml` are meant for local development, so change them before exposing a server. `docker-compose.production.yml` holds the production setup.

To run the binary without Docker, start MongoDB and PostgreSQL yourself and run:

```sh
go run ./cmd/everstead-backend
```

## Configuration

Every setting is read from the environment.

| Variable | Default |
| --- | --- |
| `EVERSTEAD_LISTEN_ADDRESS` | `:8088` |
| `EVERSTEAD_RENDEZVOUS_ADDRESS` | `:37373` |
| `EVERSTEAD_PUBLIC_HOST` | `127.0.0.1` |
| `EVERSTEAD_MONGO_URI` | `mongodb://127.0.0.1:27017` |
| `EVERSTEAD_MONGO_DATABASE` | `everstead` |
| `EVERSTEAD_POSTGRES_URL` | `postgres://everstead:everstead@127.0.0.1:5432/everstead?sslmode=disable` |
| `EVERSTEAD_DISCOURSE_URL` | `https://forum.plasnet.org` |
| `EVERSTEAD_DISCOURSE_SECRET` | empty |
| `EVERSTEAD_ADMIN_PASSWORD_HASH` | empty |
| `EVERSTEAD_ACCESS_TOKEN_TTL` | `1h` |
| `EVERSTEAD_REFRESH_TOKEN_TTL` | `720h` |
| `EVERSTEAD_SESSION_TTL` | `2m` |
| `EVERSTEAD_PRESENCE_TTL` | `2m` |
| `EVERSTEAD_INVITE_TTL` | `5m` |
| `EVERSTEAD_FRIEND_REQUEST_TTL` | `720h` |
| `EVERSTEAD_MAX_REQUEST_BYTES` | `1048576` |

`EVERSTEAD_PUBLIC_HOST` is the address clients use to reach the rendezvous, so it has to be the server's public address in production. `EVERSTEAD_ADMIN_PASSWORD_HASH` is the bcrypt hash of the built in `Admin` account's password and is required the first time the backend starts. Durations use Go's duration format, for example `30m` or `24h`.

## API

`GET /api` lists the supported API versions. The current version is `v1`, and every route is prefixed with `/v1`.

| Area | Routes |
| --- | --- |
| Status | `GET /v1/health`, `GET /v1/errors` |
| Accounts | `/v1/account/register`, `login`, `logout`, `refresh`, `me` |
| Users | `/v1/users/{pid}`, `/v1/users/name/{username}`, `/v1/users/name/{username}/look` |
| Friends | `/v1/friends` and `/v1/friends/requests` |
| Presence | `/v1/presence` |
| Invites | `/v1/invites` |
| Sessions | `/v1/sessions`, including join tickets and peers |
| Dedicated servers | `/v1/servers/name/{name}`, `/v1/server/sessions`, `/v1/server/users/name/{username}` |

`GET /v1/errors` returns the table of error codes the API can respond with.

## Tests

```sh
go test ./...
```
