package v1

import (
	"errors"
	"net/http"
	"time"

	"github.com/everstead/backend/internal/errcode"
	"github.com/everstead/backend/internal/httpx"
	"github.com/everstead/backend/internal/store"
)

type userResponse struct {
	Pid        uint32 `json:"pid"`
	Username   string `json:"username"`
	Disabled   bool   `json:"disabled"`
	State      string `json:"state"`
	Joinable   bool   `json:"joinable"`
	CurrentSid uint64 `json:"currentSid,string"`
}

type lookResponse struct {
	SkinModel string `json:"skinModel"`
	SkinHash  string `json:"skinHash"`
	Skin      []int  `json:"skin"`
	Cape      string `json:"cape"`
}

type friendRequestResponse struct {
	RequestID      string `json:"requestId"`
	SourcePid      uint32 `json:"sourcePid"`
	TargetPid      uint32 `json:"targetPid"`
	SourceUsername string `json:"sourceUsername"`
	TargetUsername string `json:"targetUsername"`
	ExpiresInDays  int    `json:"expiresInDays"`
}

type sendFriendRequestBody struct {
	TargetPid uint32 `json:"targetPid"`
}

type presenceBody struct {
	State      string `json:"state"`
	Joinable   bool   `json:"joinable"`
	CurrentSid uint64 `json:"currentSid,string"`
}

func (a *API) userByPid(writer http.ResponseWriter, request *http.Request) {
	if _, _, ok := a.account(writer, request); !ok {
		return
	}

	pid, ok := pidArgument(request)
	if !ok {
		httpx.WriteError(writer, errcode.UserNotFound)
		return
	}

	account, err := a.store.AccountByPid(request.Context(), pid)
	if err != nil {
		httpx.Fail(writer, errcode.UserNotFound, err)
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, a.describeUser(request, account))
}

func (a *API) userByName(writer http.ResponseWriter, request *http.Request) {
	if _, _, ok := a.account(writer, request); !ok {
		return
	}

	account, err := a.store.AccountByUsername(request.Context(), request.PathValue("username"))
	if err != nil {
		httpx.Fail(writer, errcode.UserNotFound, err)
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, a.describeUser(request, account))
}

func (a *API) userLook(writer http.ResponseWriter, request *http.Request) {
	if _, _, ok := a.account(writer, request); !ok {
		return
	}

	account, err := a.store.AccountByUsername(request.Context(), request.PathValue("username"))
	if err != nil {
		httpx.Fail(writer, errcode.UserNotFound, err)
		return
	}

	// Skin bytes go out as a number array because the game has no base64 decoder
	response := lookResponse{SkinModel: account.SkinModel, SkinHash: account.SkinHash, Skin: []int{}, Cape: account.Cape}
	if account.SkinHash != "" {
		data, err := a.store.Texture(request.Context(), account.SkinHash)
		if err != nil {
			httpx.Fail(writer, errcode.SkinLoad, err)
			return
		}

		for _, value := range data {
			response.Skin = append(response.Skin, int(value))
		}
	}

	httpx.WriteJSON(writer, http.StatusOK, response)
}

func (a *API) listFriends(writer http.ResponseWriter, request *http.Request) {
	account, _, ok := a.account(writer, request)
	if !ok {
		return
	}

	friends, err := a.store.Friends(request.Context(), account.Pid)
	if err != nil {
		httpx.Fail(writer, errcode.FriendsLoadFailed, err)
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, map[string]any{"friends": friends})
}

func (a *API) removeFriend(writer http.ResponseWriter, request *http.Request) {
	account, _, ok := a.account(writer, request)
	if !ok {
		return
	}

	pid, ok := pidArgument(request)
	if !ok {
		httpx.WriteError(writer, errcode.UserNotFound)
		return
	}

	if err := a.store.RemoveFriendship(request.Context(), account.Pid, pid); err != nil {
		httpx.Fail(writer, errcode.FriendsUpdate, err)
		return
	}

	writer.WriteHeader(http.StatusNoContent)
}

func (a *API) listFriendRequests(writer http.ResponseWriter, request *http.Request) {
	account, _, ok := a.account(writer, request)
	if !ok {
		return
	}

	incoming, outgoing, err := a.store.FriendRequestsFor(request.Context(), account.Pid)
	if err != nil {
		httpx.Fail(writer, errcode.FriendsLoadFailed, err)
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, map[string]any{
		"incoming": a.describeRequests(request, incoming),
		"outgoing": a.describeRequests(request, outgoing),
	})
}

func (a *API) sendFriendRequest(writer http.ResponseWriter, request *http.Request) {
	account, _, ok := a.account(writer, request)
	if !ok {
		return
	}

	body := sendFriendRequestBody{}
	if !httpx.ReadJSON(writer, request, &body, a.config.MaxRequestBytes) {
		return
	}

	if body.TargetPid == account.Pid {
		httpx.WriteError(writer, errcode.CannotAddSelf)
		return
	}

	if _, err := a.store.AccountByPid(request.Context(), body.TargetPid); err != nil {
		httpx.WriteError(writer, errcode.RequestUserMissing)
		return
	}

	friends, err := a.store.AreFriends(request.Context(), account.Pid, body.TargetPid)
	if err != nil {
		httpx.Fail(writer, errcode.FriendsLoadFailed, err)
		return
	}

	if friends {
		httpx.WriteError(writer, errcode.AlreadyFriends)
		return
	}

	created, err := a.store.CreateFriendRequest(request.Context(), account.Pid, body.TargetPid)
	if errors.Is(err, store.ErrConflict) {
		httpx.WriteJSON(writer, http.StatusOK, a.describeRequest(request, store.FriendRequest{
			SourcePid: account.Pid, TargetPid: body.TargetPid,
		}))
		return
	}

	if err != nil {
		httpx.Fail(writer, errcode.RequestUpdate, err)
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, a.describeRequest(request, *created))
}

func (a *API) getFriendRequest(writer http.ResponseWriter, request *http.Request) {
	account, _, ok := a.account(writer, request)
	if !ok {
		return
	}

	found, err := a.store.FriendRequestByID(request.Context(), request.PathValue("requestId"))
	if err != nil || (found.SourcePid != account.Pid && found.TargetPid != account.Pid) {
		httpx.WriteError(writer, errcode.RequestNotFound)
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, a.describeRequest(request, *found))
}

func (a *API) cancelFriendRequest(writer http.ResponseWriter, request *http.Request) {
	a.resolveFriendRequest(writer, request, false, false)
}

func (a *API) acceptFriendRequest(writer http.ResponseWriter, request *http.Request) {
	a.resolveFriendRequest(writer, request, true, true)
}

func (a *API) denyFriendRequest(writer http.ResponseWriter, request *http.Request) {
	a.resolveFriendRequest(writer, request, true, false)
}

func (a *API) resolveFriendRequest(writer http.ResponseWriter, request *http.Request, incomingOnly bool, befriend bool) {
	account, _, ok := a.account(writer, request)
	if !ok {
		return
	}

	found, err := a.store.FriendRequestByID(request.Context(), request.PathValue("requestId"))
	if err != nil {
		httpx.Fail(writer, errcode.RequestNotFound, err)
		return
	}

	allowed := found.TargetPid == account.Pid
	if !incomingOnly {
		allowed = found.SourcePid == account.Pid
	}

	if !allowed {
		httpx.WriteError(writer, errcode.RequestNotFound)
		return
	}

	if befriend {
		if err := a.store.AddFriendship(request.Context(), found.SourcePid, found.TargetPid); err != nil {
			httpx.WriteError(writer, errcode.FriendsUpdate)
			return
		}
	}

	if err := a.store.DeleteFriendRequest(request.Context(), found.RequestID); err != nil {
		httpx.Fail(writer, errcode.RequestUpdate, err)
		return
	}

	writer.WriteHeader(http.StatusNoContent)
}

func (a *API) publishPresence(writer http.ResponseWriter, request *http.Request) {
	account, _, ok := a.account(writer, request)
	if !ok {
		return
	}

	body := presenceBody{}
	if !httpx.ReadJSON(writer, request, &body, a.config.MaxRequestBytes) {
		return
	}

	// Dedicated server sessions are joined through the servers list, so friends never see them
	if body.CurrentSid != 0 {
		session, err := a.store.GameSessionBySid(request.Context(), body.CurrentSid)
		if err == nil && session.ServerID != "" {
			body.Joinable = false
			body.CurrentSid = 0
		}
	}

	stored, err := a.store.SetPresence(request.Context(), store.Presence{
		Pid:        account.Pid,
		State:      body.State,
		Joinable:   body.Joinable,
		CurrentSid: body.CurrentSid,
	})
	if err != nil {
		httpx.Fail(writer, errcode.PresenceUpdate, err)
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, stored)
}

func (a *API) getPresence(writer http.ResponseWriter, request *http.Request) {
	if _, _, ok := a.account(writer, request); !ok {
		return
	}

	pid, ok := pidArgument(request)
	if !ok {
		httpx.WriteError(writer, errcode.PresenceNotFound)
		return
	}

	presence, err := a.store.PresenceByPid(request.Context(), pid)
	if err != nil {
		httpx.Fail(writer, errcode.PresenceNotFound, err)
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, presence)
}

func (a *API) describeUser(request *http.Request, account *store.Account) userResponse {
	described := userResponse{Pid: account.Pid, Username: account.Username, Disabled: account.Disabled}
	presence, err := a.store.PresenceByPid(request.Context(), account.Pid)
	if err == nil {
		described.State = presence.State
		described.Joinable = presence.Joinable
		described.CurrentSid = presence.CurrentSid
	}

	return described
}

func (a *API) describeRequests(request *http.Request, requests []store.FriendRequest) []friendRequestResponse {
	described := make([]friendRequestResponse, 0, len(requests))
	for _, entry := range requests {
		described = append(described, a.describeRequest(request, entry))
	}

	return described
}

func (a *API) describeRequest(request *http.Request, entry store.FriendRequest) friendRequestResponse {
	described := friendRequestResponse{
		RequestID: entry.RequestID,
		SourcePid: entry.SourcePid,
		TargetPid: entry.TargetPid,
	}

	if !entry.ExpiresAt.IsZero() {
		described.ExpiresInDays = int(time.Until(entry.ExpiresAt).Hours() / 24)
	}

	accounts, err := a.store.AccountsByPids(request.Context(), []uint32{entry.SourcePid, entry.TargetPid})
	if err == nil {
		if source, found := accounts[entry.SourcePid]; found {
			described.SourceUsername = source.Username
		}

		if target, found := accounts[entry.TargetPid]; found {
			described.TargetUsername = target.Username
		}
	}

	return described
}
