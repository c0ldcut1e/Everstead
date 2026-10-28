package website

import (
	"crypto/hmac"
	"crypto/sha256"
	"encoding/base64"
	"encoding/hex"
	"net/http"
	"net/url"
	"strconv"

	"github.com/everstead/backend/internal/store"
)

func (w *Website) discourseSign(payload string) string {
	mac := hmac.New(sha256.New, []byte(w.config.DiscourseSecret))
	mac.Write([]byte(payload))
	return hex.EncodeToString(mac.Sum(nil))
}

func (w *Website) discourseLogin(writer http.ResponseWriter, request *http.Request) {
	if w.config.DiscourseSecret == "" {
		http.NotFound(writer, request)
		return
	}

	payload := request.URL.Query().Get("sso")
	signature, err := hex.DecodeString(request.URL.Query().Get("sig"))
	expected, _ := hex.DecodeString(w.discourseSign(payload))
	if err != nil || !hmac.Equal(signature, expected) {
		http.Error(writer, "Invalid forum login request.", http.StatusBadRequest)
		return
	}

	decoded, err := base64.StdEncoding.DecodeString(payload)
	if err != nil {
		http.Error(writer, "Invalid forum login request.", http.StatusBadRequest)
		return
	}

	values, err := url.ParseQuery(string(decoded))
	if err != nil || values.Get("nonce") == "" || values.Get("return_sso_url") != w.config.DiscourseURL+"/session/sso_login" {
		http.Error(writer, "Invalid forum login request.", http.StatusBadRequest)
		return
	}

	account := w.signedIn(request)
	if account == nil {
		http.Redirect(writer, request, "/login?next="+url.QueryEscape(request.URL.RequestURI()), http.StatusSeeOther)
		return
	}

	// Discourse trusts this email and links existing forum users by it, which is safe because account emails are unique
	response := url.Values{}
	response.Set("nonce", values.Get("nonce"))
	response.Set("external_id", strconv.FormatUint(uint64(account.Pid), 10))
	response.Set("email", account.Email)
	response.Set("username", account.Username)
	response.Set("require_activation", "false")
	response.Set("admin", strconv.FormatBool(account.Permission >= store.RoleAdmin))
	response.Set("moderator", strconv.FormatBool(account.Permission >= store.RoleModerator))

	encoded := base64.StdEncoding.EncodeToString([]byte(response.Encode()))
	target := values.Get("return_sso_url") + "?sso=" + url.QueryEscape(encoded) + "&sig=" + w.discourseSign(encoded)
	http.Redirect(writer, request, target, http.StatusFound)
}
