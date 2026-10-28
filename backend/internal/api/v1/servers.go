package v1

import (
	"errors"
	"net/http"

	"github.com/everstead/backend/internal/auth"
	"github.com/everstead/backend/internal/errcode"
	"github.com/everstead/backend/internal/httpx"
	"github.com/everstead/backend/internal/store"
)

type serverSessionBody struct {
	MaxPlayers     int `json:"maxPlayers"`
	CurrentPlayers int `json:"currentPlayers"`
}

type serverStatusResponse struct {
	Name           string `json:"name"`
	Online         bool   `json:"online"`
	Sid            uint64 `json:"sid,string"`
	Joinable       bool   `json:"joinable"`
	MaxPlayers     int    `json:"maxPlayers"`
	CurrentPlayers int    `json:"currentPlayers"`
}

func (a *API) dedicatedServer(writer http.ResponseWriter, request *http.Request) (*store.DedicatedServer, bool) {
	token := httpx.BearerToken(request)
	if token == "" {
		httpx.WriteError(writer, errcode.ServerTokenInvalid)
		return nil, false
	}

	server, err := a.store.DedicatedServerByTokenHash(request.Context(), auth.HashToken(token))
	if err != nil {
		httpx.Fail(writer, errcode.ServerTokenInvalid, err)
		return nil, false
	}

	return server, true
}

func (a *API) serverSessionFields(server *store.DedicatedServer, body serverSessionBody) store.GameSession {
	return store.GameSession{
		ServerID:       server.ServerID,
		HostUsername:   server.Name,
		Name:           server.Name,
		Visibility:     "public",
		Joinable:       body.CurrentPlayers < body.MaxPlayers,
		MaxPlayers:     body.MaxPlayers,
		CurrentPlayers: body.CurrentPlayers,
	}
}

func (a *API) openServerSession(writer http.ResponseWriter, request *http.Request) {
	server, ok := a.dedicatedServer(writer, request)
	if !ok {
		return
	}

	body := serverSessionBody{}
	if !httpx.ReadJSON(writer, request, &body, a.config.MaxRequestBytes) {
		return
	}

	created, err := a.store.CreateGameSession(request.Context(), a.serverSessionFields(server, body))
	if err != nil {
		httpx.Fail(writer, errcode.SessionCreate, err)
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, a.describeOwnedSession(created))
}

func (a *API) updateServerSession(writer http.ResponseWriter, request *http.Request) {
	server, ok := a.dedicatedServer(writer, request)
	if !ok {
		return
	}

	sid, ok := sidArgument(request)
	if !ok {
		httpx.WriteError(writer, errcode.SessionNotFound)
		return
	}

	body := serverSessionBody{}
	if !httpx.ReadJSON(writer, request, &body, a.config.MaxRequestBytes) {
		return
	}

	updated, err := a.store.UpdateGameSession(request.Context(), sid, 0, server.ServerID, a.serverSessionFields(server, body))
	if errors.Is(err, store.ErrNotFound) {
		httpx.WriteError(writer, errcode.SessionNotFound)
		return
	}

	if err != nil {
		httpx.Fail(writer, errcode.SessionUpdate, err)
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, a.describeOwnedSession(updated))
}

func (a *API) closeServerSession(writer http.ResponseWriter, request *http.Request) {
	server, ok := a.dedicatedServer(writer, request)
	if !ok {
		return
	}

	sid, ok := sidArgument(request)
	if !ok {
		httpx.WriteError(writer, errcode.SessionNotFound)
		return
	}

	if err := a.store.DeleteGameSession(request.Context(), sid, 0, server.ServerID); err != nil {
		httpx.Fail(writer, errcode.SessionNotFound, err)
		return
	}

	writer.WriteHeader(http.StatusNoContent)
}

func (a *API) serverStatus(writer http.ResponseWriter, request *http.Request) {
	if _, _, ok := a.account(writer, request); !ok {
		return
	}

	server, err := a.store.DedicatedServerByName(request.Context(), request.PathValue("name"))
	if err != nil {
		httpx.Fail(writer, errcode.ServerNotFound, err)
		return
	}

	status := serverStatusResponse{Name: server.Name}
	session, err := a.store.GameSessionByServer(request.Context(), server.ServerID)
	if errors.Is(err, store.ErrNotFound) {
		httpx.WriteJSON(writer, http.StatusOK, status)
		return
	}

	if err != nil {
		httpx.Fail(writer, errcode.SessionLoad, err)
		return
	}

	status.Online = true
	status.Sid = session.Sid
	status.Joinable = session.Joinable && session.CurrentPlayers < session.MaxPlayers
	status.MaxPlayers = session.MaxPlayers
	status.CurrentPlayers = session.CurrentPlayers
	httpx.WriteJSON(writer, http.StatusOK, status)
}

func (a *API) serverUserByName(writer http.ResponseWriter, request *http.Request) {
	if _, ok := a.dedicatedServer(writer, request); !ok {
		return
	}

	account, err := a.store.AccountByUsername(request.Context(), request.PathValue("username"))
	if err != nil {
		httpx.Fail(writer, errcode.UserNotFound, err)
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, userResponse{Pid: account.Pid, Username: account.Username, Disabled: account.Disabled})
}
