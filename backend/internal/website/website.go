// Package website serves the account pages: register, sign in and the account overview.
package website

import (
	"bytes"
	"crypto/sha256"
	"embed"
	"encoding/hex"
	"errors"
	"html/template"
	"log"
	"net/http"
	"strings"
	"time"

	"github.com/everstead/backend/internal/auth"
	"github.com/everstead/backend/internal/config"
	"github.com/everstead/backend/internal/errcode"
	"github.com/everstead/backend/internal/httpx"
	"github.com/everstead/backend/internal/store"
	"github.com/yuin/goldmark"
	"github.com/yuin/goldmark/extension"
)

//go:embed templates/*.html
var templateFiles embed.FS

//go:embed static/*
var staticFiles embed.FS

const sessionCookie = "everstead_session"

// Goldmark drops raw HTML and dangerous link schemes unless it is told to render them unsafely
var markdown = goldmark.New(goldmark.WithExtensions(extension.GFM))

type Website struct {
	store     *store.Store
	config    config.Config
	templates *template.Template

	versionCache versionCache
}

type pageData struct {
	Title   string
	Account *store.Account
	Error   string
	Notice  string
	Form    map[string]string
	Servers []store.DedicatedServer
	Token   string
	Forum   forumPage
	Admin   adminPage

	Versions []versionEntry
	Capes    []capeView
}

func renderMarkdown(text string) template.HTML {
	var buffer bytes.Buffer
	if err := markdown.Convert([]byte(text), &buffer); err != nil {
		log.Printf("website: render markdown: %v", err)
	}

	return template.HTML(buffer.String())
}

func Register(mux *http.ServeMux, database *store.Store, settings config.Config) {
	// Cloudflare caches static files for hours, so each one gets a content hash in its URL
	staticURLs := map[string]string{}
	for _, name := range []string{"style.css", "score.js", "skin.js"} {
		data, err := staticFiles.ReadFile("static/" + name)
		if err != nil {
			log.Fatalf("website: read static %s: %v", name, err)
		}

		sum := sha256.Sum256(data)
		staticURLs[name] = "/static/" + name + "?v=" + hex.EncodeToString(sum[:6])
	}

	funcs := template.FuncMap{"markdown": renderMarkdown, "static": func(name string) string { return staticURLs[name] }}
	site := &Website{
		store:     database,
		config:    settings,
		templates: template.Must(template.New("").Funcs(funcs).ParseFS(templateFiles, "templates/*.html")),
	}

	mux.Handle("GET /static/", http.FileServerFS(staticFiles))
	mux.HandleFunc("GET /{$}", site.home)
	mux.HandleFunc("GET /score", site.score)
	mux.HandleFunc("GET /versions", site.versions)
	mux.HandleFunc("GET /forum", site.forumRedirect)
	mux.HandleFunc("GET /forum/{path...}", site.forumRedirect)
	mux.HandleFunc("GET /admin", site.admin)
	mux.HandleFunc("GET /admin/account/{pid}", site.adminAccount)
	mux.HandleFunc("POST /admin/account/{pid}/{action}", site.adminAccountAction)
	mux.HandleFunc("GET /register", site.showRegister)
	mux.HandleFunc("POST /register", site.submitRegister)
	mux.HandleFunc("GET /login", site.showLogin)
	mux.HandleFunc("GET /discourse/sso", site.discourseLogin)
	mux.HandleFunc("POST /login", site.submitLogin)
	mux.HandleFunc("POST /logout", site.submitLogout)
	mux.HandleFunc("GET /account", site.account)
	mux.HandleFunc("POST /account/password", site.changePassword)
	mux.HandleFunc("POST /account/username", site.changeUsername)
	mux.HandleFunc("POST /account/email", site.changeEmail)
	mux.HandleFunc("POST /account/skin", site.changeSkin)
	mux.HandleFunc("POST /account/cape", site.changeCape)
	mux.HandleFunc("GET /textures/{hash}", site.texture)
	mux.HandleFunc("POST /account/servers", site.createServer)
	mux.HandleFunc("POST /account/servers/{serverId}/token", site.resetServerToken)
	mux.HandleFunc("POST /account/servers/{serverId}/delete", site.deleteServer)
}

func (w *Website) home(writer http.ResponseWriter, request *http.Request) {
	w.render(writer, "home.html", pageData{Title: "Everstead", Account: w.signedIn(request)})
}

func (w *Website) score(writer http.ResponseWriter, request *http.Request) {
	w.render(writer, "score.html", pageData{Title: "Bug score calculator", Account: w.signedIn(request)})
}

func (w *Website) showRegister(writer http.ResponseWriter, request *http.Request) {
	if w.signedIn(request) != nil {
		http.Redirect(writer, request, "/account", http.StatusSeeOther)
		return
	}

	w.render(writer, "register.html", pageData{Title: "Create an account"})
}

func (w *Website) submitRegister(writer http.ResponseWriter, request *http.Request) {
	if err := request.ParseForm(); err != nil {
		w.fail(writer, "register.html", "Create an account", errcode.BadRequestBody, nil)
		return
	}

	username := request.PostFormValue("username")
	email := request.PostFormValue("email")
	password := request.PostFormValue("password")
	form := map[string]string{"username": username, "email": email}

	address := httpx.ClientAddress(request)
	allowed, err := w.store.AllowRequest(request.Context(), "web-register:"+address, time.Hour, 10)
	if err != nil || !allowed {
		w.fail(writer, "register.html", "Create an account", errcode.TooManyAttempts, form)
		return
	}

	if !auth.ValidUsername(username) {
		w.fail(writer, "register.html", "Create an account", errcode.UsernameInvalid, form)
		return
	}

	if !auth.ValidEmail(email) {
		w.fail(writer, "register.html", "Create an account", errcode.EmailInvalid, form)
		return
	}

	if !auth.ValidPassword(password) {
		w.fail(writer, "register.html", "Create an account", errcode.PasswordTooShort, form)
		return
	}

	passwordHash, err := auth.HashPassword(password)
	if err != nil {
		w.fail(writer, "register.html", "Create an account", errcode.HashFailed, form)
		return
	}

	account, err := w.store.CreateAccount(request.Context(), username, email, passwordHash)
	if err != nil {
		w.fail(writer, "register.html", "Create an account", errcode.AccountExists, form)
		return
	}

	w.startSession(writer, request, account)
	http.Redirect(writer, request, "/account", http.StatusSeeOther)
}

func loginTarget(next string) string {
	if strings.HasPrefix(next, "/discourse/sso?") {
		return next
	}

	return "/account"
}

func (w *Website) showLogin(writer http.ResponseWriter, request *http.Request) {
	next := request.URL.Query().Get("next")
	if w.signedIn(request) != nil {
		http.Redirect(writer, request, loginTarget(next), http.StatusSeeOther)
		return
	}

	w.render(writer, "login.html", pageData{Title: "Sign in", Form: map[string]string{"next": next}})
}

func (w *Website) submitLogin(writer http.ResponseWriter, request *http.Request) {
	if err := request.ParseForm(); err != nil {
		w.fail(writer, "login.html", "Sign in", errcode.BadRequestBody, nil)
		return
	}

	username := request.PostFormValue("username")
	password := request.PostFormValue("password")
	next := request.PostFormValue("next")
	form := map[string]string{"username": username, "next": next}

	address := httpx.ClientAddress(request)
	allowed, err := w.store.AllowRequest(request.Context(), "web-login:"+address, 15*time.Minute, 20)
	if err != nil || !allowed {
		w.fail(writer, "login.html", "Sign in", errcode.TooManyAttempts, form)
		return
	}

	account, err := w.store.AccountByUsername(request.Context(), username)
	if err != nil || !auth.CheckPassword(account.PasswordHash, password) {
		w.store.RecordLogin(request.Context(), 0, username, address, false, errcode.BadCredentials.Code.String())
		w.fail(writer, "login.html", "Sign in", errcode.BadCredentials, form)
		return
	}

	if account.Disabled {
		w.fail(writer, "login.html", "Sign in", errcode.AccountDisabled, form)
		return
	}

	w.store.RecordLogin(request.Context(), account.Pid, account.Username, address, true, "")
	w.startSession(writer, request, account)
	http.Redirect(writer, request, loginTarget(next), http.StatusSeeOther)
}

func (w *Website) submitLogout(writer http.ResponseWriter, request *http.Request) {
	cookie, err := request.Cookie(sessionCookie)
	if err == nil {
		session, err := w.store.AuthSessionByAccessHash(request.Context(), auth.HashToken(cookie.Value))
		if err == nil {
			_ = w.store.RevokeAuthSession(request.Context(), session.ID)
		}
	}

	http.SetCookie(writer, &http.Cookie{
		Name:     sessionCookie,
		Value:    "",
		Path:     "/",
		MaxAge:   -1,
		HttpOnly: true,
		SameSite: http.SameSiteLaxMode,
	})

	http.Redirect(writer, request, "/", http.StatusSeeOther)
}

func (w *Website) account(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if account == nil {
		http.Redirect(writer, request, "/login", http.StatusSeeOther)
		return
	}

	notice := ""
	switch request.URL.Query().Get("notice") {
	case "password":
		notice = "Your password has been changed."
	case "username":
		notice = "Your username has been changed."
	case "email":
		notice = "Your email has been changed."
	case "skin":
		notice = "Your skin has been changed."
	case "cape":
		notice = "Your cape has been changed."
	case "server-deleted":
		notice = "Your server has been deleted."
	}

	w.renderAccount(writer, request, pageData{Account: account, Notice: notice})
}

func (w *Website) changePassword(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if account == nil {
		http.Redirect(writer, request, "/login", http.StatusSeeOther)
		return
	}

	if err := request.ParseForm(); err != nil {
		w.failAccount(writer, request, account, errcode.BadRequestBody)
		return
	}

	allowed, err := w.store.AllowRequest(request.Context(), "web-password:"+account.UsernameLower, time.Hour, 10)
	if err != nil || !allowed {
		w.failAccount(writer, request, account, errcode.TooManyAttempts)
		return
	}

	currentPassword := request.PostFormValue("currentPassword")
	newPassword := request.PostFormValue("newPassword")
	if !auth.CheckPassword(account.PasswordHash, currentPassword) {
		w.failAccount(writer, request, account, errcode.CurrentPasswordBad)
		return
	}

	if !auth.ValidPassword(newPassword) {
		w.failAccount(writer, request, account, errcode.PasswordTooShort)
		return
	}

	passwordHash, err := auth.HashPassword(newPassword)
	if err != nil {
		w.failAccount(writer, request, account, errcode.HashFailed)
		return
	}

	if err := w.store.SetAccountPassword(request.Context(), account.Pid, passwordHash); err != nil {
		w.failAccount(writer, request, account, errcode.SignInFailed)
		return
	}

	w.startSession(writer, request, account)
	http.Redirect(writer, request, "/account?notice=password", http.StatusSeeOther)
}

func (w *Website) changeUsername(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if account == nil {
		http.Redirect(writer, request, "/login", http.StatusSeeOther)
		return
	}

	if err := request.ParseForm(); err != nil {
		w.failAccount(writer, request, account, errcode.BadRequestBody)
		return
	}

	username := request.PostFormValue("username")
	if !auth.ValidUsername(username) {
		w.failAccount(writer, request, account, errcode.UsernameInvalid)
		return
	}

	if strings.EqualFold(username, account.Username) {
		w.failAccount(writer, request, account, errcode.AccountExists)
		return
	}

	err := w.store.ChangeAccountUsername(request.Context(), account.Pid, username)
	if errors.Is(err, store.ErrConflict) {
		w.failAccount(writer, request, account, errcode.AccountExists)
		return
	}

	if errors.Is(err, store.ErrUsernameChangeTooSoon) {
		w.failAccount(writer, request, account, errcode.UsernameChangeTooSoon)
		return
	}

	if err != nil {
		w.failAccount(writer, request, account, errcode.IdentityUpdate)
		return
	}

	http.Redirect(writer, request, "/account?notice=username", http.StatusSeeOther)
}

func (w *Website) changeEmail(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if account == nil {
		http.Redirect(writer, request, "/login", http.StatusSeeOther)
		return
	}

	if err := request.ParseForm(); err != nil {
		w.failAccount(writer, request, account, errcode.BadRequestBody)
		return
	}

	allowed, err := w.store.AllowRequest(request.Context(), "web-email:"+account.UsernameLower, time.Hour, 10)
	if err != nil || !allowed {
		w.failAccount(writer, request, account, errcode.TooManyAttempts)
		return
	}

	if !auth.CheckPassword(account.PasswordHash, request.PostFormValue("currentPassword")) {
		w.failAccount(writer, request, account, errcode.CurrentPasswordBad)
		return
	}

	email := request.PostFormValue("email")
	if !auth.ValidEmail(email) {
		w.failAccount(writer, request, account, errcode.EmailInvalid)
		return
	}

	err = w.store.SetAccountEmail(request.Context(), account.Pid, email)
	if errors.Is(err, store.ErrConflict) {
		w.failAccount(writer, request, account, errcode.AccountExists)
		return
	}

	if err != nil {
		w.failAccount(writer, request, account, errcode.IdentityUpdate)
		return
	}

	http.Redirect(writer, request, "/account?notice=email", http.StatusSeeOther)
}

func (w *Website) createServer(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if account == nil {
		http.Redirect(writer, request, "/login", http.StatusSeeOther)
		return
	}

	if err := request.ParseForm(); err != nil {
		w.failAccount(writer, request, account, errcode.BadRequestBody)
		return
	}

	allowed, err := w.store.AllowRequest(request.Context(), "web-server:"+account.UsernameLower, time.Hour, 10)
	if err != nil || !allowed {
		w.failAccount(writer, request, account, errcode.TooManyAttempts)
		return
	}

	name := request.PostFormValue("name")
	if !auth.ValidServerName(name) {
		w.failAccount(writer, request, account, errcode.ServerNameInvalid)
		return
	}

	token, tokenHash := auth.NewToken()
	_, err = w.store.CreateDedicatedServer(request.Context(), account.Pid, name, tokenHash)
	if errors.Is(err, store.ErrConflict) {
		w.failAccount(writer, request, account, errcode.ServerNameTaken)
		return
	}

	if errors.Is(err, store.ErrServerLimit) {
		w.failAccount(writer, request, account, errcode.ServerLimitReached)
		return
	}

	if err != nil {
		w.failAccount(writer, request, account, errcode.ServerUpdate)
		return
	}

	w.renderAccount(writer, request, pageData{Account: account, Notice: "Your server " + name + " has been created.", Token: token})
}

func (w *Website) resetServerToken(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if account == nil {
		http.Redirect(writer, request, "/login", http.StatusSeeOther)
		return
	}

	token, tokenHash := auth.NewToken()
	if err := w.store.ResetDedicatedServerToken(request.Context(), request.PathValue("serverId"), account.Pid, tokenHash); err != nil {
		w.failAccount(writer, request, account, errcode.ServerNotFound)
		return
	}

	w.renderAccount(writer, request, pageData{Account: account, Notice: "Your server token has been regenerated.", Token: token})
}

func (w *Website) deleteServer(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if account == nil {
		http.Redirect(writer, request, "/login", http.StatusSeeOther)
		return
	}

	if err := w.store.DeleteDedicatedServer(request.Context(), request.PathValue("serverId"), account.Pid); err != nil {
		w.failAccount(writer, request, account, errcode.ServerNotFound)
		return
	}

	http.Redirect(writer, request, "/account?notice=server-deleted", http.StatusSeeOther)
}

func (w *Website) startSession(writer http.ResponseWriter, request *http.Request, account *store.Account) {
	accessToken, accessHash := auth.NewToken()
	_, refreshHash := auth.NewToken()

	address := httpx.ClientAddress(request)
	if _, err := w.store.CreateAuthSession(request.Context(), account.Pid, store.WebsiteInstallID, address, accessHash, refreshHash); err != nil {
		log.Printf("website: create session: %v", err)
		return
	}

	http.SetCookie(writer, &http.Cookie{
		Name:     sessionCookie,
		Value:    accessToken,
		Path:     "/",
		Expires:  time.Now().Add(w.config.AccessTokenTTL),
		HttpOnly: true,
		SameSite: http.SameSiteLaxMode,
	})
}

func (w *Website) signedIn(request *http.Request) *store.Account {
	cookie, err := request.Cookie(sessionCookie)
	if err != nil {
		return nil
	}

	session, err := w.store.AuthSessionByAccessHash(request.Context(), auth.HashToken(cookie.Value))
	if err != nil {
		return nil
	}

	account, err := w.store.AccountByPid(request.Context(), session.Pid)
	if err != nil || account.Disabled {
		return nil
	}

	return account
}

func (w *Website) fail(writer http.ResponseWriter, page string, title string, apiError *errcode.Error, form map[string]string) {
	writer.WriteHeader(apiError.Status)
	w.render(writer, page, pageData{
		Title: title,
		Error: apiError.Code.String() + " - " + apiError.Message,
		Form:  form,
	})
}

func (w *Website) failAccount(writer http.ResponseWriter, request *http.Request, account *store.Account, apiError *errcode.Error) {
	writer.WriteHeader(apiError.Status)
	w.renderAccount(writer, request, pageData{Account: account, Error: apiError.Code.String() + " - " + apiError.Message})
}

func (w *Website) renderAccount(writer http.ResponseWriter, request *http.Request, data pageData) {
	servers, err := w.store.DedicatedServersByOwner(request.Context(), data.Account.Pid)
	if err != nil {
		log.Printf("website: load servers: %v", err)
	}

	data.Title = "Your account"
	data.Servers = servers
	data.Capes = capeViews(data.Account)
	w.render(writer, "account.html", data)
}

func (w *Website) render(writer http.ResponseWriter, page string, data pageData) {
	writer.Header().Set("Content-Type", "text/html; charset=utf-8")
	if err := w.templates.ExecuteTemplate(writer, page, data); err != nil {
		log.Printf("website: render %s: %v", page, err)
	}
}
