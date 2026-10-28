package v1

import (
	"net/http"

	"github.com/everstead/backend/internal/errcode"
	"github.com/everstead/backend/internal/httpx"
	"github.com/everstead/backend/internal/store"
)

type sessionBody struct {
	Name             string `json:"name"`
	Visibility       string `json:"visibility"`
	Joinable         bool   `json:"joinable"`
	FriendsOfFriends bool   `json:"friendsOfFriends"`
	MaxPlayers       int    `json:"maxPlayers"`
	CurrentPlayers   int    `json:"currentPlayers"`
}

type sessionResponse struct {
	store.GameSession
	HostKey        string `json:"hostKey,omitempty"`
	RendezvousHost string `json:"rendezvousHost,omitempty"`
	RendezvousPort int    `json:"rendezvousPort,omitempty"`
}

type peerBody struct {
	Username string `json:"username"`
	State    string `json:"state"`
}

type ticketResponse struct {
	Ticket         string `json:"ticket"`
	Sid            uint64 `json:"sid,string"`
	HostPid        uint32 `json:"hostPid"`
	RendezvousHost string `json:"rendezvousHost"`
	RendezvousPort int    `json:"rendezvousPort"`
}

func (a *API) listSessions(writer http.ResponseWriter, request *http.Request) {
	account, _, ok := a.account(writer, request)
	if !ok {
		return
	}

	friends, err := a.store.Friends(request.Context(), account.Pid)
	if err != nil {
		httpx.Fail(writer, errcode.FriendsLoadFailed, err)
		return
	}

	sessions, err := a.store.GameSessionsForHosts(request.Context(), friends)
	if err != nil {
		httpx.Fail(writer, errcode.SessionLoad, err)
		return
	}

	joined, err := a.store.GameSessionsWithPeers(request.Context(), friends)
	if err != nil {
		httpx.Fail(writer, errcode.SessionLoad, err)
		return
	}

	seen := map[uint64]bool{}
	visible := make([]sessionResponse, 0, len(sessions)+len(joined))
	for _, session := range append(sessions, joined...) {
		if session.Visibility == "private" || session.ServerID != "" || session.HostPid == account.Pid || seen[session.Sid] {
			continue
		}

		seen[session.Sid] = true
		session.Joinable = session.Joinable && a.mayJoin(request, account.Pid, &session)
		visible = append(visible, sessionResponse{GameSession: session})
	}

	httpx.WriteJSON(writer, http.StatusOK, map[string]any{"sessions": visible})
}

func (a *API) createSession(writer http.ResponseWriter, request *http.Request) {
	account, _, ok := a.account(writer, request)
	if !ok {
		return
	}

	body := sessionBody{}
	if !httpx.ReadJSON(writer, request, &body, a.config.MaxRequestBytes) {
		return
	}

	created, err := a.store.CreateGameSession(request.Context(), store.GameSession{
		HostPid:          account.Pid,
		HostUsername:     account.Username,
		Name:             body.Name,
		Visibility:       body.Visibility,
		Joinable:         body.Joinable,
		FriendsOfFriends: body.FriendsOfFriends,
		MaxPlayers:       body.MaxPlayers,
		CurrentPlayers:   body.CurrentPlayers,
	})
	if err != nil {
		httpx.Fail(writer, errcode.SessionCreate, err)
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, a.describeOwnedSession(created))
}

func (a *API) getSession(writer http.ResponseWriter, request *http.Request) {
	account, _, ok := a.account(writer, request)
	if !ok {
		return
	}

	sid, ok := sidArgument(request)
	if !ok {
		httpx.WriteError(writer, errcode.SessionNotFound)
		return
	}

	session, err := a.store.GameSessionBySid(request.Context(), sid)
	if err != nil {
		httpx.Fail(writer, errcode.SessionNotFound, err)
		return
	}

	if session.HostPid == account.Pid {
		httpx.WriteJSON(writer, http.StatusOK, a.describeOwnedSession(session))
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, sessionResponse{GameSession: *session})
}

func (a *API) updateSession(writer http.ResponseWriter, request *http.Request) {
	account, _, ok := a.account(writer, request)
	if !ok {
		return
	}

	sid, ok := sidArgument(request)
	if !ok {
		httpx.WriteError(writer, errcode.SessionNotFound)
		return
	}

	body := sessionBody{}
	if !httpx.ReadJSON(writer, request, &body, a.config.MaxRequestBytes) {
		return
	}

	updated, err := a.store.UpdateGameSession(request.Context(), sid, account.Pid, "", store.GameSession{
		Name:             body.Name,
		Visibility:       body.Visibility,
		Joinable:         body.Joinable,
		FriendsOfFriends: body.FriendsOfFriends,
		MaxPlayers:       body.MaxPlayers,
		CurrentPlayers:   body.CurrentPlayers,
		HostUsername:     account.Username,
	})
	if err != nil {
		httpx.Fail(writer, errcode.SessionUpdate, err)
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, a.describeOwnedSession(updated))
}

func (a *API) closeSession(writer http.ResponseWriter, request *http.Request) {
	account, _, ok := a.account(writer, request)
	if !ok {
		return
	}

	sid, ok := sidArgument(request)
	if !ok {
		httpx.WriteError(writer, errcode.SessionNotFound)
		return
	}

	if err := a.store.DeleteGameSession(request.Context(), sid, account.Pid, ""); err != nil {
		httpx.Fail(writer, errcode.NotSessionHost, err)
		return
	}

	writer.WriteHeader(http.StatusNoContent)
}

func (a *API) createJoinTicket(writer http.ResponseWriter, request *http.Request) {
	account, _, ok := a.account(writer, request)
	if !ok {
		return
	}

	sid, ok := sidArgument(request)
	if !ok {
		httpx.WriteError(writer, errcode.SessionNotFound)
		return
	}

	session, err := a.store.GameSessionBySid(request.Context(), sid)
	if err != nil {
		httpx.Fail(writer, errcode.SessionNotFound, err)
		return
	}

	if !session.Joinable || session.CurrentPlayers >= session.MaxPlayers {
		httpx.WriteError(writer, errcode.SessionNotJoinable)
		return
	}

	if !a.mayJoin(request, account.Pid, session) {
		httpx.WriteError(writer, errcode.TicketNotYours)
		return
	}

	ticket, err := a.store.CreateJoinTicket(request.Context(), session, account.Pid)
	if err != nil {
		httpx.Fail(writer, errcode.TicketCreate, err)
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, ticketResponse{
		Ticket:         ticket.Ticket,
		Sid:            session.Sid,
		HostPid:        session.HostPid,
		RendezvousHost: a.config.PublicHost,
		RendezvousPort: a.rendezvousPort(),
	})
}

func (a *API) listPeers(writer http.ResponseWriter, request *http.Request) {
	if _, _, ok := a.account(writer, request); !ok {
		return
	}

	sid, ok := sidArgument(request)
	if !ok {
		httpx.WriteError(writer, errcode.SessionNotFound)
		return
	}

	peers, err := a.store.SessionPeers(request.Context(), sid)
	if err != nil {
		httpx.Fail(writer, errcode.PeerLoad, err)
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, map[string]any{"peers": peers})
}

func (a *API) publishPeer(writer http.ResponseWriter, request *http.Request) {
	account, _, ok := a.account(writer, request)
	if !ok {
		return
	}

	sid, ok := sidArgument(request)
	if !ok {
		httpx.WriteError(writer, errcode.SessionNotFound)
		return
	}

	if _, err := a.store.GameSessionBySid(request.Context(), sid); err != nil {
		httpx.WriteError(writer, errcode.SessionNotFound)
		return
	}

	body := peerBody{}
	if !httpx.ReadJSON(writer, request, &body, a.config.MaxRequestBytes) {
		return
	}

	stored, err := a.store.UpsertSessionPeer(request.Context(), store.SessionPeer{
		Sid:      sid,
		Pid:      account.Pid,
		Username: account.Username,
		State:    body.State,
	})
	if err != nil {
		httpx.Fail(writer, errcode.PeerUpdate, err)
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, stored)
}

func (a *API) removePeer(writer http.ResponseWriter, request *http.Request) {
	account, _, ok := a.account(writer, request)
	if !ok {
		return
	}

	sid, ok := sidArgument(request)
	if !ok {
		httpx.WriteError(writer, errcode.SessionNotFound)
		return
	}

	if err := a.store.DeleteSessionPeer(request.Context(), sid, account.Pid); err != nil {
		httpx.Fail(writer, errcode.PeerUpdate, err)
		return
	}

	writer.WriteHeader(http.StatusNoContent)
}

func (a *API) mayJoin(request *http.Request, pid uint32, session *store.GameSession) bool {
	if session.HostPid == pid {
		return true
	}

	if session.Visibility == "public" {
		return true
	}

	incoming, _, err := a.store.InvitesFor(request.Context(), pid)
	if err == nil {
		for _, invite := range incoming {
			if invite.Sid == session.Sid {
				return true
			}
		}
	}

	if session.Visibility != "friends" {
		return false
	}

	friends, err := a.store.AreFriends(request.Context(), pid, session.HostPid)
	if err != nil || friends || !session.FriendsOfFriends {
		return err == nil && friends
	}

	peers, err := a.store.SessionPeers(request.Context(), session.Sid)
	if err != nil {
		return false
	}

	for _, peer := range peers {
		if friends, err := a.store.AreFriends(request.Context(), pid, peer.Pid); err == nil && friends {
			return true
		}
	}

	return false
}

func (a *API) describeOwnedSession(session *store.GameSession) sessionResponse {
	return sessionResponse{
		GameSession:    *session,
		HostKey:        session.HostKey,
		RendezvousHost: a.config.PublicHost,
		RendezvousPort: a.rendezvousPort(),
	}
}
