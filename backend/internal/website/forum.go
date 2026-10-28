package website

import (
	"context"
	"errors"
	"fmt"
	"log"
	"net/http"
	"regexp"
	"strconv"
	"strings"
	"time"
	"unicode/utf8"

	"github.com/everstead/backend/internal/errcode"
	"github.com/everstead/backend/internal/store"
	"go.mongodb.org/mongo-driver/bson"
)

const forumPageSize = 25

var linkPattern = regexp.MustCompile(`(?i)https?://|www\.`)

type forumCategory struct {
	Slug        string
	Name        string
	Description string
	Statuses    []string
	Votes       bool
	Placeholder string
}

var forumCategories = []forumCategory{
	{"bugs", "Bug reports", "Report crashes, glitches and exploits.", []string{"Open", "Confirmed", "Fixed", "Won't fix", "Duplicate"}, true,
		"What happened, the steps to reproduce it, what you expected instead, your game version and platform."},
	{"suggestions", "Suggestions", "Ideas for new features and changes.", []string{"Open", "Planned", "Added", "Declined", "Duplicate"}, true,
		"Describe your idea, why it would make the game better and how it should work."},
	{"help", "Help", "Questions about playing, servers and accounts.", nil, false,
		"What do you need help with? Include what you already tried."},
	{"general", "General", "Everything else about Everstead.", nil, false,
		"Write your post."},
}

var muteDurations = map[string]time.Duration{"1h": time.Hour, "1d": 24 * time.Hour, "7d": 7 * 24 * time.Hour, "30d": 30 * 24 * time.Hour}

var permanentMute = time.Date(9999, 1, 1, 0, 0, 0, 0, time.UTC)

type forumAuthor struct {
	Name  string
	Badge string
}

type forumCategoryView struct {
	forumCategory
	ThreadCount int64
	Latest      *forumThreadView
}

type forumThreadView struct {
	store.ForumThread
	Author forumAuthor
}

type forumPostView struct {
	store.ForumPost
	Author    forumAuthor
	CanEdit   bool
	CanDelete bool
}

type forumReportView struct {
	store.ForumReport
	Reporter    string
	ThreadTitle string
	Excerpt     string
}

type modLogView struct {
	store.ModLogEntry
	Actor  string
	Target string
}

type forumProfile struct {
	Account *store.Account
	Badge   string
	Stats   store.ForumUserStats
	Muted   bool
	CanMute bool
}

type forumPage struct {
	Categories  []forumCategoryView
	Category    *forumCategory
	Threads     []forumThreadView
	Thread      *forumThreadView
	Posts       []forumPostView
	Post        *forumPostView
	Reports     []forumReportView
	Log         []modLogView
	Profile     *forumProfile
	Query       string
	Top         bool
	Voted       bool
	Page        int
	PrevPage    int
	NextPage    int
	Pages       int
	CanModerate bool
	CanDelete   bool
}

func forumCategoryBySlug(slug string) *forumCategory {
	for i := range forumCategories {
		if forumCategories[i].Slug == slug {
			return &forumCategories[i]
		}
	}

	return nil
}

func canModerate(account *store.Account) bool {
	return account != nil && account.Permission >= store.RoleModerator
}

// The built in Admin can manage other admins, everyone else can only manage accounts below their own role
func canManage(actor *store.Account, target *store.Account) bool {
	return actor.Pid != target.Pid && !target.BuiltIn && (actor.BuiltIn || actor.Permission > target.Permission)
}

func accountBadge(account *store.Account) string {
	if account.ForumTitle != "" {
		return account.ForumTitle
	}

	switch account.Permission {
	case store.RoleAdmin:
		return "Admin"
	case store.RoleModerator:
		return "Moderator"
	}

	return ""
}

func isMuted(account *store.Account) bool {
	return account.MutedUntil.After(time.Now())
}

func threadURL(threadID int64) string {
	return "/forum/thread/" + strconv.FormatInt(threadID, 10)
}

func (w *Website) forumAuthors(ctx context.Context, pids []uint32) map[uint32]forumAuthor {
	accounts, err := w.store.AccountsByPids(ctx, pids)
	if err != nil {
		log.Printf("website: load forum authors: %v", err)
	}

	authors := map[uint32]forumAuthor{}
	for _, pid := range pids {
		authors[pid] = forumAuthor{Name: "Deleted account"}
		if account, found := accounts[pid]; found {
			authors[pid] = forumAuthor{Name: account.Username, Badge: accountBadge(account)}
		}
	}

	return authors
}

func (w *Website) threadViews(ctx context.Context, threads []store.ForumThread) []forumThreadView {
	pids := make([]uint32, len(threads))
	for i, thread := range threads {
		pids[i] = thread.AuthorPid
	}

	authors := w.forumAuthors(ctx, pids)
	views := make([]forumThreadView, len(threads))
	for i, thread := range threads {
		views[i] = forumThreadView{ForumThread: thread, Author: authors[thread.AuthorPid]}
	}

	return views
}

func (w *Website) addModLog(ctx context.Context, entry store.ModLogEntry) {
	if err := w.store.AddModLog(ctx, entry); err != nil {
		log.Printf("website: write mod log: %v", err)
	}
}

func (w *Website) forumRedirect(writer http.ResponseWriter, request *http.Request) {
	http.Redirect(writer, request, "https://forum.plasnet.org", http.StatusFound)
}

func (w *Website) forum(writer http.ResponseWriter, request *http.Request) {
	categories := make([]forumCategoryView, len(forumCategories))
	pids := []uint32{}
	for i, category := range forumCategories {
		latest, total, err := w.store.LatestForumThread(request.Context(), category.Slug)
		if err != nil {
			log.Printf("website: load forum category %s: %v", category.Slug, err)
		}

		categories[i] = forumCategoryView{forumCategory: category, ThreadCount: total}
		if latest != nil {
			categories[i].Latest = &forumThreadView{ForumThread: *latest}
			pids = append(pids, latest.AuthorPid)
		}
	}

	authors := w.forumAuthors(request.Context(), pids)
	for i := range categories {
		if categories[i].Latest != nil {
			categories[i].Latest.Author = authors[categories[i].Latest.AuthorPid]
		}
	}

	account := w.signedIn(request)
	w.render(writer, "forum.html", pageData{Title: "Forum", Account: account, Forum: forumPage{Categories: categories, CanModerate: canModerate(account)}})
}

func (w *Website) forumCategory(writer http.ResponseWriter, request *http.Request) {
	category := forumCategoryBySlug(request.PathValue("category"))
	if category == nil {
		http.NotFound(writer, request)
		return
	}

	page, err := strconv.Atoi(request.URL.Query().Get("page"))
	if err != nil || page < 1 {
		page = 1
	}

	top := category.Votes && request.URL.Query().Get("sort") == "top"
	threads, total, err := w.store.ForumThreads(request.Context(), category.Slug, top, int64(page-1)*forumPageSize, forumPageSize)
	if err != nil {
		log.Printf("website: load forum threads: %v", err)
	}

	w.render(writer, "forum_category.html", pageData{
		Title:   category.Name,
		Account: w.signedIn(request),
		Forum: forumPage{
			Category: category,
			Threads:  w.threadViews(request.Context(), threads),
			Top:      top,
			Page:     page,
			PrevPage: page - 1,
			NextPage: page + 1,
			Pages:    max(1, int((total+forumPageSize-1)/forumPageSize)),
		},
	})
}

func (w *Website) forumSearch(writer http.ResponseWriter, request *http.Request) {
	query := strings.TrimSpace(request.URL.Query().Get("q"))
	threads := []store.ForumThread{}
	if utf8.RuneCountInString(query) >= 2 {
		found, err := w.store.SearchForum(request.Context(), query, 50)
		if err != nil {
			log.Printf("website: search forum: %v", err)
		}

		threads = found
	}

	w.render(writer, "forum_search.html", pageData{
		Title:   "Search",
		Account: w.signedIn(request),
		Forum:   forumPage{Query: query, Threads: w.threadViews(request.Context(), threads)},
	})
}

func (w *Website) forumUser(writer http.ResponseWriter, request *http.Request) {
	user, err := w.store.AccountByUsername(request.Context(), request.PathValue("username"))
	if err != nil {
		http.NotFound(writer, request)
		return
	}

	stats, err := w.store.ForumUserStats(request.Context(), user.Pid)
	if err != nil {
		log.Printf("website: load forum stats: %v", err)
	}

	threads, err := w.store.ForumThreadsByAuthor(request.Context(), user.Pid, 20)
	if err != nil {
		log.Printf("website: load forum threads: %v", err)
	}

	account := w.signedIn(request)
	w.render(writer, "forum_user.html", pageData{
		Title:   user.Username,
		Account: account,
		Notice:  forumNotice(request),
		Forum: forumPage{
			Threads:     w.threadViews(request.Context(), threads),
			CanModerate: canModerate(account),
			Profile: &forumProfile{
				Account: user,
				Badge:   accountBadge(user),
				Stats:   stats,
				Muted:   isMuted(user),
				CanMute: canModerate(account) && canManage(account, user),
			},
		},
	})
}

func (w *Website) muteForumUser(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if !canModerate(account) {
		http.NotFound(writer, request)
		return
	}

	pid, err := strconv.ParseUint(request.PathValue("pid"), 10, 32)
	if err != nil {
		http.NotFound(writer, request)
		return
	}

	target, err := w.store.AccountByPid(request.Context(), uint32(pid))
	if err != nil {
		http.NotFound(writer, request)
		return
	}

	if err := request.ParseForm(); err != nil {
		http.Error(writer, errcode.BadRequestBody.Message, errcode.BadRequestBody.Status)
		return
	}

	if !canManage(account, target) {
		http.Error(writer, errcode.AdminProtected.Message, errcode.AdminProtected.Status)
		return
	}

	duration := request.PostFormValue("duration")
	reason := strings.TrimSpace(request.PostFormValue("reason"))
	until := time.Time{}
	action := "unmute"
	if duration == "permanent" {
		until = permanentMute
	} else if length, found := muteDurations[duration]; found {
		until = time.Now().Add(length)
	} else if duration != "none" {
		http.Error(writer, errcode.AdminInvalid.Message, errcode.AdminInvalid.Status)
		return
	}

	if !until.IsZero() {
		action = "mute"
	}

	if utf8.RuneCountInString(reason) > 200 {
		http.Error(writer, errcode.AdminInvalid.Message, errcode.AdminInvalid.Status)
		return
	}

	if err := w.store.SetAccountFields(request.Context(), target.Pid, bson.M{"mutedUntil": until, "muteReason": reason}); err != nil {
		http.Error(writer, errcode.AdminUpdate.Message, errcode.AdminUpdate.Status)
		return
	}

	w.addModLog(request.Context(), store.ModLogEntry{ActorPid: account.Pid, Action: action, TargetPid: target.Pid, Detail: strings.TrimSpace(duration + " " + reason)})

	destination := request.PostFormValue("return")
	if !strings.HasPrefix(destination, "/") || strings.HasPrefix(destination, "//") {
		destination = "/forum/user/" + target.Username
	}

	http.Redirect(writer, request, destination+"?notice=account-updated", http.StatusSeeOther)
}

func (w *Website) showForumThread(writer http.ResponseWriter, request *http.Request) {
	thread := w.requestForumThread(writer, request)
	if thread == nil {
		return
	}

	w.renderForumThread(writer, request, thread, pageData{Account: w.signedIn(request), Notice: forumNotice(request)})
}

func (w *Website) renderForumThread(writer http.ResponseWriter, request *http.Request, thread *store.ForumThread, data pageData) {
	moderator := canModerate(data.Account)
	if thread.Deleted && !moderator {
		http.NotFound(writer, request)
		return
	}

	posts, err := w.store.ForumPosts(request.Context(), thread.ThreadID)
	if err != nil {
		log.Printf("website: load forum posts: %v", err)
	}

	pids := []uint32{thread.AuthorPid}
	for _, post := range posts {
		pids = append(pids, post.AuthorPid)
	}

	authors := w.forumAuthors(request.Context(), pids)
	views := make([]forumPostView, len(posts))
	for i, post := range posts {
		own := data.Account != nil && data.Account.Pid == post.AuthorPid
		views[i] = forumPostView{
			ForumPost: post,
			Author:    authors[post.AuthorPid],
			CanEdit:   moderator || (own && !post.Deleted && !thread.Locked),
			CanDelete: !post.First && (own || moderator),
		}
	}

	voted := false
	if data.Account != nil {
		voted, err = w.store.HasForumVote(request.Context(), thread.ThreadID, data.Account.Pid)
		if err != nil {
			log.Printf("website: load forum vote: %v", err)
		}
	}

	data.Title = thread.Title
	data.Forum = forumPage{
		Categories:  forumCategoryViews(),
		Category:    forumCategoryBySlug(thread.Category),
		Thread:      &forumThreadView{ForumThread: *thread, Author: authors[thread.AuthorPid]},
		Posts:       views,
		Voted:       voted,
		CanModerate: moderator,
		CanDelete:   moderator || (data.Account != nil && data.Account.Pid == thread.AuthorPid),
	}
	w.render(writer, "forum_thread.html", data)
}

func (w *Website) showForumNew(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if account == nil {
		http.Redirect(writer, request, "/login", http.StatusSeeOther)
		return
	}

	category := forumCategoryBySlug(request.URL.Query().Get("category"))
	if category == nil {
		category = &forumCategories[0]
	}

	w.render(writer, "forum_new.html", pageData{
		Title:   "New thread",
		Account: account,
		Form:    map[string]string{"category": category.Slug},
		Forum:   forumPage{Categories: forumCategoryViews()},
	})
}

func (w *Website) submitForumNew(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if account == nil {
		http.Redirect(writer, request, "/login", http.StatusSeeOther)
		return
	}

	if err := request.ParseForm(); err != nil {
		w.failForumNew(writer, account, errcode.BadRequestBody, nil)
		return
	}

	title := strings.TrimSpace(request.PostFormValue("title"))
	body := strings.TrimSpace(request.PostFormValue("body"))
	form := map[string]string{"category": request.PostFormValue("category"), "title": title, "body": body}
	category := forumCategoryBySlug(form["category"])
	if category == nil {
		w.failForumNew(writer, account, errcode.BadRequestBody, form)
		return
	}

	if length := utf8.RuneCountInString(title); length < 3 || length > 100 {
		w.failForumNew(writer, account, errcode.ForumTitleInvalid, form)
		return
	}

	if apiError := w.checkForumPost(request, account, title+"\n"+body, body); apiError != nil {
		w.failForumNew(writer, account, apiError, form)
		return
	}

	status := ""
	if len(category.Statuses) > 0 {
		status = category.Statuses[0]
	}

	thread, err := w.store.CreateForumThread(request.Context(), store.ForumThread{Category: category.Slug, Title: title, AuthorPid: account.Pid, Status: status}, body)
	if err != nil {
		w.failForumNew(writer, account, errcode.ForumUpdate, form)
		return
	}

	http.Redirect(writer, request, threadURL(thread.ThreadID), http.StatusSeeOther)
}

func (w *Website) failForumNew(writer http.ResponseWriter, account *store.Account, apiError *errcode.Error, form map[string]string) {
	writer.WriteHeader(apiError.Status)
	w.render(writer, "forum_new.html", pageData{
		Title:   "New thread",
		Account: account,
		Error:   apiError.Code.String() + " - " + apiError.Message,
		Form:    form,
		Forum:   forumPage{Categories: forumCategoryViews()},
	})
}

func (w *Website) submitForumReply(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if account == nil {
		http.Redirect(writer, request, "/login", http.StatusSeeOther)
		return
	}

	thread := w.requestForumThread(writer, request)
	if thread == nil {
		return
	}

	if err := request.ParseForm(); err != nil {
		w.failForumThread(writer, request, account, thread, errcode.BadRequestBody, "")
		return
	}

	body := strings.TrimSpace(request.PostFormValue("body"))
	if (thread.Locked || thread.Deleted) && !canModerate(account) {
		w.failForumThread(writer, request, account, thread, errcode.ForumLocked, body)
		return
	}

	if apiError := w.checkForumPost(request, account, body, body); apiError != nil {
		w.failForumThread(writer, request, account, thread, apiError, body)
		return
	}

	if err := w.store.CreateForumPost(request.Context(), thread.ThreadID, account.Pid, body); err != nil {
		w.failForumThread(writer, request, account, thread, errcode.ForumUpdate, body)
		return
	}

	http.Redirect(writer, request, threadURL(thread.ThreadID)+"#latest", http.StatusSeeOther)
}

func (w *Website) voteForumThread(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if account == nil {
		http.Redirect(writer, request, "/login", http.StatusSeeOther)
		return
	}

	thread := w.requestForumThread(writer, request)
	if thread == nil {
		return
	}

	if !forumCategoryBySlug(thread.Category).Votes || thread.Deleted || account.Disabled {
		w.failForumThread(writer, request, account, thread, errcode.ForumForbidden, "")
		return
	}

	if err := w.store.ToggleForumVote(request.Context(), thread.ThreadID, account.Pid); err != nil {
		w.failForumThread(writer, request, account, thread, errcode.ForumUpdate, "")
		return
	}

	http.Redirect(writer, request, threadURL(thread.ThreadID), http.StatusSeeOther)
}

func (w *Website) deleteForumThread(writer http.ResponseWriter, request *http.Request) {
	w.setForumThreadDeleted(writer, request, true)
}

func (w *Website) restoreForumThread(writer http.ResponseWriter, request *http.Request) {
	w.setForumThreadDeleted(writer, request, false)
}

func (w *Website) setForumThreadDeleted(writer http.ResponseWriter, request *http.Request, deleted bool) {
	account := w.signedIn(request)
	if account == nil {
		http.Redirect(writer, request, "/login", http.StatusSeeOther)
		return
	}

	thread := w.requestForumThread(writer, request)
	if thread == nil {
		return
	}

	if !canModerate(account) && (!deleted || thread.AuthorPid != account.Pid) {
		w.failForumThread(writer, request, account, thread, errcode.ForumForbidden, "")
		return
	}

	if err := w.store.SetForumThreadDeleted(request.Context(), thread.ThreadID, deleted, account.Pid); err != nil {
		w.failForumThread(writer, request, account, thread, errcode.ForumUpdate, "")
		return
	}

	action := "restore thread"
	if deleted {
		action = "delete thread"
	}

	w.addModLog(request.Context(), store.ModLogEntry{ActorPid: account.Pid, Action: action, TargetPid: thread.AuthorPid, ThreadID: thread.ThreadID, Detail: thread.Title})
	if deleted && !canModerate(account) {
		http.Redirect(writer, request, "/forum/"+thread.Category, http.StatusSeeOther)
		return
	}

	http.Redirect(writer, request, threadURL(thread.ThreadID)+"?notice=thread-updated", http.StatusSeeOther)
}

func (w *Website) deleteForumPost(writer http.ResponseWriter, request *http.Request) {
	w.setForumPostDeleted(writer, request, true)
}

func (w *Website) restoreForumPost(writer http.ResponseWriter, request *http.Request) {
	w.setForumPostDeleted(writer, request, false)
}

func (w *Website) setForumPostDeleted(writer http.ResponseWriter, request *http.Request, deleted bool) {
	account := w.signedIn(request)
	if account == nil {
		http.Redirect(writer, request, "/login", http.StatusSeeOther)
		return
	}

	post, thread := w.requestForumPost(writer, request)
	if post == nil {
		return
	}

	// The first post is the thread itself, so it can only go away by deleting the thread
	if post.First || (!canModerate(account) && (!deleted || post.AuthorPid != account.Pid)) {
		w.failForumThread(writer, request, account, thread, errcode.ForumForbidden, "")
		return
	}

	if err := w.store.SetForumPostDeleted(request.Context(), post, deleted, account.Pid); err != nil {
		w.failForumThread(writer, request, account, thread, errcode.ForumUpdate, "")
		return
	}

	action := "restore post"
	notice := "post-restored"
	if deleted {
		action = "delete post"
		notice = "post-deleted"
	}

	w.addModLog(request.Context(), store.ModLogEntry{ActorPid: account.Pid, Action: action, TargetPid: post.AuthorPid, ThreadID: thread.ThreadID, PostID: post.PostID})
	http.Redirect(writer, request, threadURL(thread.ThreadID)+"?notice="+notice, http.StatusSeeOther)
}

func (w *Website) showForumEdit(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if account == nil {
		http.Redirect(writer, request, "/login", http.StatusSeeOther)
		return
	}

	post, thread := w.requestForumPost(writer, request)
	if post == nil {
		return
	}

	w.renderForumEdit(writer, account, post, thread, nil, map[string]string{"title": thread.Title, "body": post.Body})
}

func (w *Website) submitForumEdit(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if account == nil {
		http.Redirect(writer, request, "/login", http.StatusSeeOther)
		return
	}

	post, thread := w.requestForumPost(writer, request)
	if post == nil {
		return
	}

	if err := request.ParseForm(); err != nil {
		w.renderForumEdit(writer, account, post, thread, errcode.BadRequestBody, nil)
		return
	}

	title := strings.TrimSpace(request.PostFormValue("title"))
	body := strings.TrimSpace(request.PostFormValue("body"))
	form := map[string]string{"title": title, "body": body}
	moderator := canModerate(account)
	if !moderator && (post.AuthorPid != account.Pid || post.Deleted || thread.Locked) {
		w.renderForumEdit(writer, account, post, thread, errcode.ForumForbidden, form)
		return
	}

	if length := utf8.RuneCountInString(title); post.First && (length < 3 || length > 100) {
		w.renderForumEdit(writer, account, post, thread, errcode.ForumTitleInvalid, form)
		return
	}

	if apiError := w.checkForumPost(request, account, title+"\n"+body, body); apiError != nil {
		w.renderForumEdit(writer, account, post, thread, apiError, form)
		return
	}

	if err := w.store.EditForumPost(request.Context(), post.PostID, body); err != nil {
		w.renderForumEdit(writer, account, post, thread, errcode.ForumUpdate, form)
		return
	}

	if post.First && title != thread.Title {
		if err := w.store.UpdateForumThread(request.Context(), thread.ThreadID, bson.M{"title": title}); err != nil {
			w.renderForumEdit(writer, account, post, thread, errcode.ForumUpdate, form)
			return
		}
	}

	if post.AuthorPid != account.Pid {
		w.addModLog(request.Context(), store.ModLogEntry{ActorPid: account.Pid, Action: "edit post", TargetPid: post.AuthorPid, ThreadID: thread.ThreadID, PostID: post.PostID})
	}

	http.Redirect(writer, request, threadURL(thread.ThreadID)+"#post-"+post.PostID, http.StatusSeeOther)
}

func (w *Website) renderForumEdit(writer http.ResponseWriter, account *store.Account, post *store.ForumPost, thread *store.ForumThread, apiError *errcode.Error, form map[string]string) {
	data := pageData{Title: "Edit post", Account: account, Form: form, Forum: forumPage{Thread: &forumThreadView{ForumThread: *thread}, Post: &forumPostView{ForumPost: *post}}}
	if apiError != nil {
		writer.WriteHeader(apiError.Status)
		data.Error = apiError.Code.String() + " - " + apiError.Message
	}

	w.render(writer, "forum_edit.html", data)
}

func (w *Website) reportForumPost(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if account == nil {
		http.Redirect(writer, request, "/login", http.StatusSeeOther)
		return
	}

	post, thread := w.requestForumPost(writer, request)
	if post == nil {
		return
	}

	if err := request.ParseForm(); err != nil {
		w.failForumThread(writer, request, account, thread, errcode.BadRequestBody, "")
		return
	}

	reason := strings.TrimSpace(request.PostFormValue("reason"))
	if length := utf8.RuneCountInString(reason); length < 1 || length > 500 {
		w.failForumThread(writer, request, account, thread, errcode.ForumReportBad, "")
		return
	}

	allowed, err := w.store.AllowRequest(request.Context(), "web-report:"+account.UsernameLower, 10*time.Minute, 5)
	if err != nil || !allowed || account.Disabled {
		w.failForumThread(writer, request, account, thread, errcode.TooManyAttempts, "")
		return
	}

	report := store.ForumReport{ReporterPid: account.Pid, ThreadID: thread.ThreadID, PostID: post.PostID, Reason: reason}
	if err := w.store.CreateForumReport(request.Context(), report); err != nil {
		w.failForumThread(writer, request, account, thread, errcode.ForumUpdate, "")
		return
	}

	http.Redirect(writer, request, threadURL(thread.ThreadID)+"?notice=reported", http.StatusSeeOther)
}

func (w *Website) forumModeration(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if !canModerate(account) {
		http.NotFound(writer, request)
		return
	}

	reports, err := w.store.OpenForumReports(request.Context())
	if err != nil {
		log.Printf("website: load forum reports: %v", err)
	}

	entries, err := w.store.ModLog(request.Context(), 0, 100)
	if err != nil {
		log.Printf("website: load mod log: %v", err)
	}

	w.render(writer, "forum_moderation.html", pageData{
		Title:   "Moderation",
		Account: account,
		Notice:  forumNotice(request),
		Forum:   forumPage{Reports: w.reportViews(request.Context(), reports), Log: w.modLogViews(request.Context(), entries)},
	})
}

func (w *Website) reportViews(ctx context.Context, reports []store.ForumReport) []forumReportView {
	pids := make([]uint32, len(reports))
	for i, report := range reports {
		pids[i] = report.ReporterPid
	}

	authors := w.forumAuthors(ctx, pids)
	views := make([]forumReportView, len(reports))
	for i, report := range reports {
		views[i] = forumReportView{ForumReport: report, Reporter: authors[report.ReporterPid].Name}
		if thread, err := w.store.ForumThreadByID(ctx, report.ThreadID); err == nil {
			views[i].ThreadTitle = thread.Title
		}

		if post, err := w.store.ForumPostByID(ctx, report.PostID); err == nil {
			views[i].Excerpt = post.Body
			if utf8.RuneCountInString(post.Body) > 200 {
				views[i].Excerpt = string([]rune(post.Body)[:200]) + "..."
			}
		}
	}

	return views
}

func (w *Website) modLogViews(ctx context.Context, entries []store.ModLogEntry) []modLogView {
	pids := []uint32{}
	for _, entry := range entries {
		pids = append(pids, entry.ActorPid, entry.TargetPid)
	}

	authors := w.forumAuthors(ctx, pids)
	views := make([]modLogView, len(entries))
	for i, entry := range entries {
		views[i] = modLogView{ModLogEntry: entry, Actor: authors[entry.ActorPid].Name, Target: authors[entry.TargetPid].Name}
	}

	return views
}

func (w *Website) resolveForumReport(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if !canModerate(account) {
		http.NotFound(writer, request)
		return
	}

	report, err := w.store.ResolveForumReport(request.Context(), request.PathValue("reportId"), account.Pid)
	if errors.Is(err, store.ErrNotFound) {
		http.NotFound(writer, request)
		return
	}

	if err != nil {
		http.Error(writer, errcode.ForumUpdate.Message, errcode.ForumUpdate.Status)
		return
	}

	w.addModLog(request.Context(), store.ModLogEntry{ActorPid: account.Pid, Action: "resolve report", TargetPid: report.ReporterPid, ThreadID: report.ThreadID, PostID: report.PostID, Detail: report.Reason})
	http.Redirect(writer, request, "/forum/moderation?notice=report-resolved", http.StatusSeeOther)
}

func (w *Website) moderateForumThread(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if account == nil {
		http.Redirect(writer, request, "/login", http.StatusSeeOther)
		return
	}

	thread := w.requestForumThread(writer, request)
	if thread == nil {
		return
	}

	if !canModerate(account) {
		w.failForumThread(writer, request, account, thread, errcode.ForumForbidden, "")
		return
	}

	if err := request.ParseForm(); err != nil {
		w.failForumThread(writer, request, account, thread, errcode.BadRequestBody, "")
		return
	}

	category := forumCategoryBySlug(request.PostFormValue("category"))
	if category == nil {
		w.failForumThread(writer, request, account, thread, errcode.BadRequestBody, "")
		return
	}

	status := request.PostFormValue("status")
	valid := len(category.Statuses) == 0 && status == ""
	for _, allowed := range category.Statuses {
		valid = valid || status == allowed
	}

	if !valid && category.Slug != thread.Category {
		status = ""
		if len(category.Statuses) > 0 {
			status = category.Statuses[0]
		}

		valid = true
	}

	if !valid {
		w.failForumThread(writer, request, account, thread, errcode.BadRequestBody, "")
		return
	}

	locked := request.PostFormValue("locked") == "on"
	duplicateOf := int64(0)
	if text := strings.TrimPrefix(strings.TrimSpace(request.PostFormValue("duplicateOf")), "#"); text != "" {
		parsed, err := strconv.ParseInt(text, 10, 64)
		if err != nil || parsed == thread.ThreadID {
			w.failForumThread(writer, request, account, thread, errcode.ForumDuplicateBad, "")
			return
		}

		if _, err := w.store.ForumThreadByID(request.Context(), parsed); err != nil {
			w.failForumThread(writer, request, account, thread, errcode.ForumDuplicateBad, "")
			return
		}

		duplicateOf = parsed
		locked = true
		if len(category.Statuses) > 0 {
			status = "Duplicate"
		}
	}

	fields := bson.M{"category": category.Slug, "status": status, "locked": locked, "pinned": request.PostFormValue("pinned") == "on", "duplicateOf": duplicateOf}
	if err := w.store.UpdateForumThread(request.Context(), thread.ThreadID, fields); err != nil {
		w.failForumThread(writer, request, account, thread, errcode.ForumUpdate, "")
		return
	}

	detail := fmt.Sprintf("category %s, status %q, locked %t, pinned %t", category.Slug, status, locked, fields["pinned"])
	if duplicateOf != 0 {
		detail += fmt.Sprintf(", duplicate of #%d", duplicateOf)
	}

	w.addModLog(request.Context(), store.ModLogEntry{ActorPid: account.Pid, Action: "update thread", TargetPid: thread.AuthorPid, ThreadID: thread.ThreadID, Detail: detail})
	http.Redirect(writer, request, threadURL(thread.ThreadID)+"?notice=thread-updated", http.StatusSeeOther)
}

func (w *Website) requestForumThread(writer http.ResponseWriter, request *http.Request) *store.ForumThread {
	threadID, err := strconv.ParseInt(request.PathValue("threadId"), 10, 64)
	if err != nil {
		http.NotFound(writer, request)
		return nil
	}

	thread, err := w.store.ForumThreadByID(request.Context(), threadID)
	if errors.Is(err, store.ErrNotFound) {
		http.NotFound(writer, request)
		return nil
	}

	if err != nil {
		http.Error(writer, errcode.ForumUpdate.Message, errcode.ForumUpdate.Status)
		return nil
	}

	return thread
}

func (w *Website) requestForumPost(writer http.ResponseWriter, request *http.Request) (*store.ForumPost, *store.ForumThread) {
	post, err := w.store.ForumPostByID(request.Context(), request.PathValue("postId"))
	if err != nil {
		http.NotFound(writer, request)
		return nil, nil
	}

	thread, err := w.store.ForumThreadByID(request.Context(), post.ThreadID)
	if err != nil {
		http.NotFound(writer, request)
		return nil, nil
	}

	return post, thread
}

// The text is everything the post shows, which is the title plus the body for a new thread
func (w *Website) checkForumPost(request *http.Request, account *store.Account, text string, body string) *errcode.Error {
	if account.Disabled {
		return errcode.ForumForbidden
	}

	if isMuted(account) {
		until := "permanently"
		if account.MutedUntil.Before(permanentMute) {
			until = "until " + account.MutedUntil.UTC().Format("2006-01-02 15:04") + " UTC"
		}

		message := "You are muted on the forum " + until + "."
		if account.MuteReason != "" {
			message += " Reason: " + account.MuteReason
		}

		return &errcode.Error{Code: errcode.ForumMuted.Code, Message: message, Status: errcode.ForumMuted.Status}
	}

	if length := utf8.RuneCountInString(body); length < 1 || length > 10000 {
		return errcode.ForumBodyInvalid
	}

	moderator := canModerate(account)
	age := time.Since(account.CreatedAt)
	if !moderator && age < 10*time.Minute {
		return errcode.ForumAccountNew
	}

	newAccount := !moderator && age < 24*time.Hour
	if newAccount && len(linkPattern.FindAllStringIndex(text, -1)) > 2 {
		return errcode.ForumTooManyLinks
	}

	limit := 10
	if newAccount {
		limit = 3
	}

	allowed, err := w.store.AllowRequest(request.Context(), "web-forum:"+account.UsernameLower, 10*time.Minute, limit)
	if err != nil || !allowed {
		return errcode.TooManyAttempts
	}

	return nil
}

func (w *Website) failForumThread(writer http.ResponseWriter, request *http.Request, account *store.Account, thread *store.ForumThread, apiError *errcode.Error, body string) {
	writer.WriteHeader(apiError.Status)
	w.renderForumThread(writer, request, thread, pageData{
		Account: account,
		Error:   apiError.Code.String() + " - " + apiError.Message,
		Form:    map[string]string{"body": body},
	})
}

func forumCategoryViews() []forumCategoryView {
	views := make([]forumCategoryView, len(forumCategories))
	for i, category := range forumCategories {
		views[i] = forumCategoryView{forumCategory: category}
	}

	return views
}

func forumNotice(request *http.Request) string {
	switch request.URL.Query().Get("notice") {
	case "post-deleted":
		return "The post has been deleted."
	case "post-restored":
		return "The post has been restored."
	case "thread-updated":
		return "The thread has been updated."
	case "reported":
		return "Thanks, a moderator will look at your report."
	case "report-resolved":
		return "The report has been resolved."
	case "account-updated":
		return "The account has been updated."
	case "account-deleted":
		return "The account has been deleted."
	}

	return ""
}
