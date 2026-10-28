package v1

import (
	"errors"
	"net/http"
	"time"

	"github.com/everstead/backend/internal/auth"
	"github.com/everstead/backend/internal/errcode"
	"github.com/everstead/backend/internal/httpx"
	"github.com/everstead/backend/internal/store"
)

type registerRequest struct {
	Username  string `json:"username"`
	Password  string `json:"password"`
	Email     string `json:"email"`
	InstallID string `json:"installId"`
}

type loginRequest struct {
	Username  string `json:"username"`
	Password  string `json:"password"`
	InstallID string `json:"installId"`
}

type refreshRequest struct {
	RefreshToken string `json:"refreshToken"`
}

type authResponse struct {
	AccessToken  string         `json:"accessToken"`
	RefreshToken string         `json:"refreshToken"`
	InstallID    string         `json:"installId"`
	Account      *store.Account `json:"account"`
}

func (a *API) register(writer http.ResponseWriter, request *http.Request) {
	address := httpx.ClientAddress(request)
	if !a.allow(writer, request, "register:"+address, time.Hour, 10) {
		return
	}

	body := registerRequest{}
	if !httpx.ReadJSON(writer, request, &body, a.config.MaxRequestBytes) {
		return
	}

	if !auth.ValidUsername(body.Username) {
		httpx.WriteError(writer, errcode.UsernameInvalid)
		return
	}

	if !auth.ValidEmail(body.Email) {
		httpx.WriteError(writer, errcode.EmailInvalid)
		return
	}

	if !auth.ValidPassword(body.Password) {
		httpx.WriteError(writer, errcode.PasswordTooShort)
		return
	}

	passwordHash, err := auth.HashPassword(body.Password)
	if err != nil {
		httpx.Fail(writer, errcode.HashFailed, err)
		return
	}

	account, err := a.store.CreateAccount(request.Context(), body.Username, body.Email, passwordHash)
	if errors.Is(err, store.ErrConflict) {
		httpx.WriteError(writer, errcode.AccountExists)
		return
	}

	if err != nil {
		httpx.Fail(writer, errcode.PidCreateFailed, err)
		return
	}

	a.startSession(writer, request, account, body.InstallID, address)
}

func (a *API) login(writer http.ResponseWriter, request *http.Request) {
	address := httpx.ClientAddress(request)
	if !a.allow(writer, request, "login:"+address, 15*time.Minute, 20) {
		return
	}

	body := loginRequest{}
	if !httpx.ReadJSON(writer, request, &body, a.config.MaxRequestBytes) {
		return
	}

	account, err := a.store.AccountByUsername(request.Context(), body.Username)
	if err != nil {
		a.store.RecordLogin(request.Context(), 0, body.Username, address, false, errcode.BadCredentials.Code.String())
		httpx.WriteError(writer, errcode.BadCredentials)
		return
	}

	if !auth.CheckPassword(account.PasswordHash, body.Password) {
		a.store.RecordLogin(request.Context(), account.Pid, account.Username, address, false, errcode.BadCredentials.Code.String())
		httpx.WriteError(writer, errcode.BadCredentials)
		return
	}

	if account.Disabled {
		a.store.RecordLogin(request.Context(), account.Pid, account.Username, address, false, errcode.AccountDisabled.Code.String())
		httpx.WriteError(writer, errcode.AccountDisabled)
		return
	}

	a.startSession(writer, request, account, body.InstallID, address)
}

func (a *API) logout(writer http.ResponseWriter, request *http.Request) {
	account, session, ok := a.account(writer, request)
	if !ok {
		return
	}

	_ = a.store.RevokeAuthSession(request.Context(), session.ID)
	_ = a.store.ClearPresence(request.Context(), account.Pid)
	writer.WriteHeader(http.StatusNoContent)
}

func (a *API) refresh(writer http.ResponseWriter, request *http.Request) {
	body := refreshRequest{}
	if !httpx.ReadJSON(writer, request, &body, a.config.MaxRequestBytes) {
		return
	}

	session, err := a.store.AuthSessionByRefreshHash(request.Context(), auth.HashToken(body.RefreshToken))
	if err != nil {
		httpx.Fail(writer, errcode.RefreshTokenBad, err)
		return
	}

	account, err := a.store.AccountByPid(request.Context(), session.Pid)
	if err != nil {
		httpx.Fail(writer, errcode.AccountNotFound, err)
		return
	}

	if account.Disabled {
		httpx.WriteError(writer, errcode.AccountDisabled)
		return
	}

	accessToken, accessHash := auth.NewToken()
	refreshToken, refreshHash := auth.NewToken()
	if err := a.store.RotateAuthSession(request.Context(), session.ID, accessHash, refreshHash); err != nil {
		httpx.Fail(writer, errcode.SignInFailed, err)
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, authResponse{
		AccessToken:  accessToken,
		RefreshToken: refreshToken,
		InstallID:    session.InstallID,
		Account:      account,
	})
}

func (a *API) me(writer http.ResponseWriter, request *http.Request) {
	account, _, ok := a.account(writer, request)
	if !ok {
		return
	}

	httpx.WriteJSON(writer, http.StatusOK, account)
}

func (a *API) startSession(writer http.ResponseWriter, request *http.Request, account *store.Account,
	installID string, address string) {
	accessToken, accessHash := auth.NewToken()
	refreshToken, refreshHash := auth.NewToken()

	session, err := a.store.CreateAuthSession(request.Context(), account.Pid, installID, address, accessHash, refreshHash)
	if err != nil {
		httpx.Fail(writer, errcode.SignInFailed, err)
		return
	}

	if err := a.store.LinkAccountIdentity(request.Context(), account.Pid, installID, address); err != nil {
		httpx.Fail(writer, errcode.IdentityUpdate, err)
		return
	}

	a.store.RecordLogin(request.Context(), account.Pid, account.Username, address, true, "")
	httpx.WriteJSON(writer, http.StatusOK, authResponse{
		AccessToken:  accessToken,
		RefreshToken: refreshToken,
		InstallID:    session.InstallID,
		Account:      account,
	})
}
