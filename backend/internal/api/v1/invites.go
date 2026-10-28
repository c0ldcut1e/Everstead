package v1

import (
	"net/http"
	"time"

	"github.com/everstead/backend/internal/errcode"
	"github.com/everstead/backend/internal/httpx"
	"github.com/everstead/backend/internal/store"
)

type createInviteBody struct {
	TargetPid uint32 `json:"targetPid"`
	Sid       uint64 `json:"sid,string"`
}

type inviteResponse struct {
	InviteID         string `json:"inviteId"`
	SourcePid        uint32 `json:"sourcePid"`
	TargetPid        uint32 `json:"targetPid"`
	Sid              uint64 `json:"sid,string"`
	SourceUsername   string `json:"sourceUsername"`
	TargetUsername   string `json:"targetUsername"`
	ExpiresInSeconds int    `json:"expiresInSeconds"`
}

func (a *API) listInvites(writer http.ResponseWriter, request *http.Request) {
	account, _, ok := a.account(writer, request)
	if !ok {
		return
	}

	incoming, outgoing, err := a.store.InvitesFor(request.Context(), account.Pid)
	if err != nil {
		httpx.Fail(writer, errcode.InviteLoad, err)
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, map[string]any{
		"incoming": a.describeInvites(request, incoming),
		"outgoing": a.describeInvites(request, outgoing),
	})
}

func (a *API) createInvite(writer http.ResponseWriter, request *http.Request) {
	account, _, ok := a.account(writer, request)
	if !ok {
		return
	}

	body := createInviteBody{}
	if !httpx.ReadJSON(writer, request, &body, a.config.MaxRequestBytes) {
		return
	}

	if body.TargetPid == account.Pid {
		httpx.WriteError(writer, errcode.CannotInviteMe)
		return
	}

	if _, err := a.store.AccountByPid(request.Context(), body.TargetPid); err != nil {
		httpx.WriteError(writer, errcode.InviteUserGone)
		return
	}

	if _, err := a.store.GameSessionBySid(request.Context(), body.Sid); err != nil {
		httpx.WriteError(writer, errcode.SessionNotFound)
		return
	}

	created, err := a.store.CreateInvite(request.Context(), account.Pid, body.TargetPid, body.Sid)
	if err != nil {
		httpx.Fail(writer, errcode.InviteUpdate, err)
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, a.describeInvite(request, *created))
}

func (a *API) getInvite(writer http.ResponseWriter, request *http.Request) {
	account, _, ok := a.account(writer, request)
	if !ok {
		return
	}

	invite, err := a.store.InviteByID(request.Context(), request.PathValue("inviteId"))
	if err != nil || (invite.SourcePid != account.Pid && invite.TargetPid != account.Pid) {
		httpx.WriteError(writer, errcode.InviteNotFound)
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, a.describeInvite(request, *invite))
}

func (a *API) cancelInvite(writer http.ResponseWriter, request *http.Request) {
	a.resolveInvite(writer, request, false, false)
}

func (a *API) acceptInvite(writer http.ResponseWriter, request *http.Request) {
	a.resolveInvite(writer, request, true, true)
}

func (a *API) denyInvite(writer http.ResponseWriter, request *http.Request) {
	a.resolveInvite(writer, request, true, false)
}

func (a *API) resolveInvite(writer http.ResponseWriter, request *http.Request, incomingOnly bool, answer bool) {
	account, _, ok := a.account(writer, request)
	if !ok {
		return
	}

	invite, err := a.store.InviteByID(request.Context(), request.PathValue("inviteId"))
	if err != nil {
		httpx.Fail(writer, errcode.InviteNotFound, err)
		return
	}

	allowed := invite.TargetPid == account.Pid
	if !incomingOnly {
		allowed = invite.SourcePid == account.Pid
	}

	if !allowed {
		httpx.WriteError(writer, errcode.InviteNotYours)
		return
	}

	if answer && time.Now().After(invite.ExpiresAt) {
		httpx.WriteError(writer, errcode.InviteExpired)
		return
	}

	if err := a.store.DeleteInvite(request.Context(), invite.InviteID); err != nil {
		httpx.Fail(writer, errcode.InviteUpdate, err)
		return
	}

	if !answer {
		writer.WriteHeader(http.StatusNoContent)
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, a.describeInvite(request, *invite))
}

func (a *API) describeInvites(request *http.Request, invites []store.Invite) []inviteResponse {
	described := make([]inviteResponse, 0, len(invites))
	for _, entry := range invites {
		described = append(described, a.describeInvite(request, entry))
	}

	return described
}

func (a *API) describeInvite(request *http.Request, entry store.Invite) inviteResponse {
	described := inviteResponse{
		InviteID:         entry.InviteID,
		SourcePid:        entry.SourcePid,
		TargetPid:        entry.TargetPid,
		Sid:              entry.Sid,
		ExpiresInSeconds: int(time.Until(entry.ExpiresAt).Seconds()),
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
