// Package v1 serves version 1 of the Everstead API under /v1.
package v1

import (
	"net"
	"net/http"
	"strconv"
	"time"

	"github.com/everstead/backend/internal/auth"
	"github.com/everstead/backend/internal/config"
	"github.com/everstead/backend/internal/errcode"
	"github.com/everstead/backend/internal/httpx"
	"github.com/everstead/backend/internal/store"
)

const Version = "v1"

type API struct {
	store  *store.Store
	config config.Config
}

func Register(mux *http.ServeMux, database *store.Store, settings config.Config) {
	api := &API{store: database, config: settings}

	mux.HandleFunc("GET /"+Version+"/health", api.health)
	mux.HandleFunc("GET /"+Version+"/errors", api.errorTable)

	mux.HandleFunc("POST /"+Version+"/account/register", api.register)
	mux.HandleFunc("POST /"+Version+"/account/login", api.login)
	mux.HandleFunc("POST /"+Version+"/account/logout", api.logout)
	mux.HandleFunc("POST /"+Version+"/account/refresh", api.refresh)
	mux.HandleFunc("GET /"+Version+"/account/me", api.me)

	mux.HandleFunc("GET /"+Version+"/users/{pid}", api.userByPid)
	mux.HandleFunc("GET /"+Version+"/users/name/{username}", api.userByName)
	mux.HandleFunc("GET /"+Version+"/users/name/{username}/look", api.userLook)

	mux.HandleFunc("GET /"+Version+"/friends", api.listFriends)
	mux.HandleFunc("DELETE /"+Version+"/friends/{pid}", api.removeFriend)
	mux.HandleFunc("GET /"+Version+"/friends/requests", api.listFriendRequests)
	mux.HandleFunc("POST /"+Version+"/friends/requests", api.sendFriendRequest)
	mux.HandleFunc("GET /"+Version+"/friends/requests/{requestId}", api.getFriendRequest)
	mux.HandleFunc("DELETE /"+Version+"/friends/requests/{requestId}", api.cancelFriendRequest)
	mux.HandleFunc("POST /"+Version+"/friends/requests/{requestId}/accept", api.acceptFriendRequest)
	mux.HandleFunc("POST /"+Version+"/friends/requests/{requestId}/deny", api.denyFriendRequest)

	mux.HandleFunc("POST /"+Version+"/presence", api.publishPresence)
	mux.HandleFunc("GET /"+Version+"/presence/{pid}", api.getPresence)

	mux.HandleFunc("GET /"+Version+"/invites", api.listInvites)
	mux.HandleFunc("POST /"+Version+"/invites", api.createInvite)
	mux.HandleFunc("GET /"+Version+"/invites/{inviteId}", api.getInvite)
	mux.HandleFunc("DELETE /"+Version+"/invites/{inviteId}", api.cancelInvite)
	mux.HandleFunc("POST /"+Version+"/invites/{inviteId}/accept", api.acceptInvite)
	mux.HandleFunc("POST /"+Version+"/invites/{inviteId}/deny", api.denyInvite)

	mux.HandleFunc("GET /"+Version+"/sessions", api.listSessions)
	mux.HandleFunc("POST /"+Version+"/sessions", api.createSession)
	mux.HandleFunc("GET /"+Version+"/sessions/{sid}", api.getSession)
	mux.HandleFunc("PUT /"+Version+"/sessions/{sid}", api.updateSession)
	mux.HandleFunc("DELETE /"+Version+"/sessions/{sid}", api.closeSession)
	mux.HandleFunc("POST /"+Version+"/sessions/{sid}/tickets", api.createJoinTicket)
	mux.HandleFunc("GET /"+Version+"/sessions/{sid}/peers", api.listPeers)
	mux.HandleFunc("PUT /"+Version+"/sessions/{sid}/peers/me", api.publishPeer)
	mux.HandleFunc("DELETE /"+Version+"/sessions/{sid}/peers/me", api.removePeer)

	mux.HandleFunc("GET /"+Version+"/servers/name/{name}", api.serverStatus)
	mux.HandleFunc("POST /"+Version+"/server/sessions", api.openServerSession)
	mux.HandleFunc("PUT /"+Version+"/server/sessions/{sid}", api.updateServerSession)
	mux.HandleFunc("DELETE /"+Version+"/server/sessions/{sid}", api.closeServerSession)
	mux.HandleFunc("GET /"+Version+"/server/users/name/{username}", api.serverUserByName)
}

func (a *API) health(writer http.ResponseWriter, request *http.Request) {
	mongoState, postgresState := a.store.Health(request.Context())
	status := "up"
	if mongoState != "up" || postgresState != "up" {
		status = "degraded"
	}

	httpx.WriteJSON(writer, http.StatusOK, map[string]string{
		"status":   status,
		"mongo":    mongoState,
		"postgres": postgresState,
	})
}

func (a *API) errorTable(writer http.ResponseWriter, _ *http.Request) {
	httpx.WriteJSON(writer, http.StatusOK, map[string]any{"codes": errcode.All()})
}

func (a *API) account(writer http.ResponseWriter, request *http.Request) (*store.Account, *store.AuthSession, bool) {
	token := httpx.BearerToken(request)
	if token == "" {
		httpx.WriteError(writer, errcode.SignInRequired)
		return nil, nil, false
	}

	session, err := a.store.AuthSessionByAccessHash(request.Context(), auth.HashToken(token))
	if err != nil {
		httpx.Fail(writer, errcode.AccessTokenInvalid, err)
		return nil, nil, false
	}

	account, err := a.store.AccountByPid(request.Context(), session.Pid)
	if err != nil {
		httpx.Fail(writer, errcode.AccountNotFound, err)
		return nil, nil, false
	}

	if account.Disabled {
		httpx.WriteError(writer, errcode.AccountDisabled)
		return nil, nil, false
	}

	return account, session, true
}

func (a *API) allow(writer http.ResponseWriter, request *http.Request, bucket string, window time.Duration, limit int) bool {
	allowed, err := a.store.AllowRequest(request.Context(), bucket, window, limit)
	if err != nil {
		httpx.Fail(writer, errcode.Unavailable, err)
		return false
	}

	if !allowed {
		httpx.WriteError(writer, errcode.TooManyAttempts)
		return false
	}

	return true
}

func (a *API) rendezvousPort() int {
	_, port, err := net.SplitHostPort(a.config.RendezvousAddress)
	if err != nil {
		return 0
	}

	number, err := strconv.Atoi(port)
	if err != nil {
		return 0
	}

	return number
}

func pidArgument(request *http.Request) (uint32, bool) {
	value, err := strconv.ParseUint(request.PathValue("pid"), 10, 32)
	if err != nil {
		return 0, false
	}

	return uint32(value), true
}

func sidArgument(request *http.Request) (uint64, bool) {
	value, err := strconv.ParseUint(request.PathValue("sid"), 10, 64)
	if err != nil {
		return 0, false
	}

	return value, true
}
