package website

import (
	"errors"
	"log"
	"net/http"
	"slices"
	"strconv"
	"strings"
	"unicode/utf8"

	"github.com/everstead/backend/internal/auth"
	"github.com/everstead/backend/internal/errcode"
	"github.com/everstead/backend/internal/store"
	"go.mongodb.org/mongo-driver/bson"
)

type adminPage struct {
	Accounts     []store.Account
	AccountCount int64
	ReportCount  int
	Query        string
	Target       *store.Account
	Servers      []store.DedicatedServer
	Sessions     []store.AuthSession
	Logins       []store.LoginRecord
	Log          []modLogView
	Stats        store.ForumUserStats
	Muted        bool
	CanManage    bool
	Password     string
}

func isAdmin(account *store.Account) bool {
	return account != nil && account.Permission >= store.RoleAdmin
}

func (w *Website) admin(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if !isAdmin(account) {
		http.NotFound(writer, request)
		return
	}

	query := strings.TrimSpace(request.URL.Query().Get("q"))
	accounts, err := w.store.SearchAccounts(request.Context(), query, 50)
	if err != nil {
		log.Printf("website: search accounts: %v", err)
	}

	count, err := w.store.CountAccounts(request.Context())
	if err != nil {
		log.Printf("website: count accounts: %v", err)
	}

	reports, err := w.store.OpenForumReports(request.Context())
	if err != nil {
		log.Printf("website: load forum reports: %v", err)
	}

	w.render(writer, "admin.html", pageData{
		Title:   "Admin",
		Account: account,
		Notice:  forumNotice(request),
		Admin:   adminPage{Accounts: accounts, AccountCount: count, ReportCount: len(reports), Query: query},
	})
}

func (w *Website) adminAccount(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if !isAdmin(account) {
		http.NotFound(writer, request)
		return
	}

	target := w.requestAdminTarget(writer, request)
	if target == nil {
		return
	}

	w.renderAdminAccount(writer, request, account, target, pageData{Notice: forumNotice(request)})
}

func (w *Website) renderAdminAccount(writer http.ResponseWriter, request *http.Request, account *store.Account, target *store.Account, data pageData) {
	ctx := request.Context()
	servers, err := w.store.DedicatedServersByOwner(ctx, target.Pid)
	if err != nil {
		log.Printf("website: load servers: %v", err)
	}

	sessions, err := w.store.ActiveAuthSessions(ctx, target.Pid)
	if err != nil {
		log.Printf("website: load sessions: %v", err)
	}

	logins, err := w.store.LoginHistory(ctx, target.Pid, 20)
	if err != nil {
		log.Printf("website: load login history: %v", err)
	}

	entries, err := w.store.ModLog(ctx, target.Pid, 50)
	if err != nil {
		log.Printf("website: load mod log: %v", err)
	}

	stats, err := w.store.ForumUserStats(ctx, target.Pid)
	if err != nil {
		log.Printf("website: load forum stats: %v", err)
	}

	data.Title = target.Username
	data.Account = account
	data.Admin.Target = target
	data.Admin.Servers = servers
	data.Admin.Sessions = sessions
	data.Admin.Logins = logins
	data.Admin.Log = w.modLogViews(ctx, entries)
	data.Admin.Stats = stats
	data.Admin.Muted = isMuted(target)
	data.Admin.CanManage = canManage(account, target)
	data.Capes = capeViews(target)
	w.render(writer, "admin_account.html", data)
}

func (w *Website) requestAdminTarget(writer http.ResponseWriter, request *http.Request) *store.Account {
	pid, err := strconv.ParseUint(request.PathValue("pid"), 10, 32)
	if err != nil {
		http.NotFound(writer, request)
		return nil
	}

	target, err := w.store.AccountByPid(request.Context(), uint32(pid))
	if err != nil {
		http.NotFound(writer, request)
		return nil
	}

	return target
}

func (w *Website) failAdminAccount(writer http.ResponseWriter, request *http.Request, account *store.Account, target *store.Account, apiError *errcode.Error) {
	writer.WriteHeader(apiError.Status)
	w.renderAdminAccount(writer, request, account, target, pageData{Error: apiError.Code.String() + " - " + apiError.Message})
}

func (w *Website) adminAccountAction(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if !isAdmin(account) {
		http.NotFound(writer, request)
		return
	}

	target := w.requestAdminTarget(writer, request)
	if target == nil {
		return
	}

	if err := request.ParseForm(); err != nil {
		w.failAdminAccount(writer, request, account, target, errcode.BadRequestBody)
		return
	}

	ctx := request.Context()
	action := request.PathValue("action")
	value := strings.TrimSpace(request.PostFormValue("value"))
	entry := store.ModLogEntry{ActorPid: account.Pid, Action: action, TargetPid: target.Pid}

	// Titles and capes are cosmetic, so admins can also set them on themselves and on the built in account
	if action != "title" && action != "capes" && !canManage(account, target) {
		w.failAdminAccount(writer, request, account, target, errcode.AdminProtected)
		return
	}

	var err error
	switch action {
	case "role":
		role, parseErr := strconv.Atoi(value)
		if parseErr != nil || role < store.RoleUser || role > store.RoleAdmin || (role == store.RoleAdmin && !account.BuiltIn) {
			w.failAdminAccount(writer, request, account, target, errcode.AdminInvalid)
			return
		}

		err = w.store.SetAccountFields(ctx, target.Pid, bson.M{"permission": role})
		entry.Detail = value
	case "title":
		if utf8.RuneCountInString(value) > 24 {
			w.failAdminAccount(writer, request, account, target, errcode.AdminInvalid)
			return
		}

		err = w.store.SetAccountFields(ctx, target.Pid, bson.M{"forumTitle": value})
		entry.Detail = value
	case "capes":
		unlocked := []string{}
		for _, entry := range capes {
			if slices.Contains(request.PostForm["cape"], entry.ID) {
				unlocked = append(unlocked, entry.ID)
			}
		}

		fields := bson.M{"capes": unlocked}
		if !slices.Contains(unlocked, target.Cape) {
			fields["cape"] = ""
		}

		err = w.store.SetAccountFields(ctx, target.Pid, fields)
		entry.Detail = strings.Join(unlocked, " ")
	case "disable", "enable":
		err = w.store.SetAccountFields(ctx, target.Pid, bson.M{"disabled": action == "disable"})
		if err == nil && action == "disable" {
			err = w.revokeEverySession(request, target.Pid)
		}
	case "username":
		if !auth.ValidUsername(value) {
			w.failAdminAccount(writer, request, account, target, errcode.UsernameInvalid)
			return
		}

		err = w.store.SetAccountFields(ctx, target.Pid, bson.M{"username": value, "usernameLower": strings.ToLower(value)})
		entry.Detail = target.Username + " to " + value
	case "email":
		if !auth.ValidEmail(value) {
			w.failAdminAccount(writer, request, account, target, errcode.EmailInvalid)
			return
		}

		err = w.store.SetAccountFields(ctx, target.Pid, bson.M{"email": value, "emailLower": strings.ToLower(value)})
		entry.Detail = target.Email + " to " + value
	case "password":
		password, _ := auth.NewToken()
		password = password[:20]
		hash, hashErr := auth.HashPassword(password)
		if hashErr != nil {
			w.failAdminAccount(writer, request, account, target, errcode.HashFailed)
			return
		}

		if err := w.store.SetAccountPassword(ctx, target.Pid, hash); err != nil {
			w.failAdminAccount(writer, request, account, target, errcode.AdminUpdate)
			return
		}

		if err := w.revokeEverySession(request, target.Pid); err != nil {
			w.failAdminAccount(writer, request, account, target, errcode.AdminUpdate)
			return
		}

		w.addModLog(ctx, entry)
		w.renderAdminAccount(writer, request, account, target, pageData{Admin: adminPage{Password: password}})
		return
	case "sessions":
		err = w.revokeEverySession(request, target.Pid)
	case "purge":
		count, purgeErr := w.store.DeleteForumPostsByAuthor(ctx, target.Pid, account.Pid)
		err = purgeErr
		entry.Detail = strconv.Itoa(count) + " replies and every thread"
	case "delete":
		if value != target.Username {
			w.failAdminAccount(writer, request, account, target, errcode.AdminConfirm)
			return
		}

		if err := w.store.DeleteAccount(ctx, target.Pid); err != nil {
			log.Printf("website: delete account %d: %v", target.Pid, err)
			w.failAdminAccount(writer, request, account, target, errcode.AdminUpdate)
			return
		}

		entry.Detail = target.Username + " " + target.Email
		w.addModLog(ctx, entry)
		http.Redirect(writer, request, "/admin?notice=account-deleted", http.StatusSeeOther)
		return
	default:
		http.NotFound(writer, request)
		return
	}

	if errors.Is(err, store.ErrConflict) {
		w.failAdminAccount(writer, request, account, target, errcode.AccountExists)
		return
	}

	if err != nil {
		log.Printf("website: admin %s on %d: %v", action, target.Pid, err)
		w.failAdminAccount(writer, request, account, target, errcode.AdminUpdate)
		return
	}

	w.addModLog(ctx, entry)
	http.Redirect(writer, request, "/admin/account/"+strconv.FormatUint(uint64(target.Pid), 10)+"?notice=account-updated", http.StatusSeeOther)
}

func (w *Website) revokeEverySession(request *http.Request, pid uint32) error {
	if err := w.store.RevokeAllAccountSessions(request.Context(), pid); err != nil {
		return err
	}

	return w.store.DeleteHostSessions(request.Context(), pid)
}
