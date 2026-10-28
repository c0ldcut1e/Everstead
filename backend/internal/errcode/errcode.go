// Package errcode defines the XXX-YYYY error codes the API and the game share.
package errcode

import (
	"fmt"
	"net/http"
	"sort"
)

type Group uint16

const (
	GroupSystem   Group = 100
	GroupRate     Group = 110
	GroupAuth     Group = 120
	GroupAccount  Group = 200
	GroupIdentity Group = 210
	GroupSocial   Group = 300
	GroupSession  Group = 400
	GroupInvite   Group = 500
	GroupServer   Group = 600
	GroupForum    Group = 700
	GroupAdmin    Group = 800
	GroupClient   Group = 900
)

type Code struct {
	Group  Group
	Number uint16
}

func (c Code) String() string {
	return fmt.Sprintf("%03d-%04d", c.Group, c.Number)
}

type Error struct {
	Code    Code
	Message string
	Status  int
}

func (e *Error) Error() string {
	return e.Code.String() + ": " + e.Message
}

var registry = map[string]*Error{}

func define(group Group, number uint16, status int, message string) *Error {
	err := &Error{Code: Code{Group: group, Number: number}, Message: message, Status: status}
	registry[err.Code.String()] = err
	return err
}

var (
	Unknown          = define(GroupSystem, 0, http.StatusInternalServerError, "Something went wrong. Please try again.")
	Unavailable      = define(GroupSystem, 1, http.StatusServiceUnavailable, "The service is currently unavailable. Please try again later.")
	BadRequestBody   = define(GroupSystem, 2, http.StatusBadRequest, "The request could not be read.")
	MethodNotAllowed = define(GroupSystem, 3, http.StatusMethodNotAllowed, "This action is not supported.")
	NotFound         = define(GroupSystem, 4, http.StatusNotFound, "Not found.")
	RequestTooLarge  = define(GroupSystem, 5, http.StatusRequestEntityTooLarge, "The request is too large.")
)

var (
	TooManyAttempts = define(GroupRate, 1, http.StatusTooManyRequests, "Too many attempts. Please wait and try again.")
)

var (
	SignInRequired     = define(GroupAuth, 1, http.StatusUnauthorized, "Sign in required.")
	AccessTokenInvalid = define(GroupAuth, 2, http.StatusUnauthorized, "Please sign in again.")
	RefreshTokenBad    = define(GroupAuth, 3, http.StatusUnauthorized, "Please sign in again.")
	SignedInElsewhere  = define(GroupAuth, 4, http.StatusUnauthorized, "This account was logged in somewhere else.")
)

var (
	UsernameInvalid       = define(GroupAccount, 1, http.StatusBadRequest, "Invalid username.")
	PasswordTooShort      = define(GroupAccount, 2, http.StatusBadRequest, "Password is too short.")
	AccountRequest        = define(GroupAccount, 3, http.StatusBadRequest, "Invalid request.")
	AccountExists         = define(GroupAccount, 4, http.StatusConflict, "Username or email already exists.")
	AccountNotFound       = define(GroupAccount, 5, http.StatusNotFound, "Account not found.")
	AccountDisabled       = define(GroupAccount, 6, http.StatusForbidden, "This account is disabled.")
	BadCredentials        = define(GroupAccount, 7, http.StatusUnauthorized, "Invalid username or password.")
	PidCreateFailed       = define(GroupAccount, 8, http.StatusInternalServerError, "Could not create account. Please try again.")
	HashFailed            = define(GroupAccount, 9, http.StatusInternalServerError, "Could not create account. Please try again.")
	SignInFailed          = define(GroupAccount, 10, http.StatusInternalServerError, "Could not sign in. Please try again.")
	EmailInvalid          = define(GroupAccount, 11, http.StatusBadRequest, "Invalid email address.")
	CurrentPasswordBad    = define(GroupAccount, 12, http.StatusUnauthorized, "Current password is incorrect.")
	UsernameChangeTooSoon = define(GroupAccount, 13, http.StatusConflict, "You can change your username once every 30 days.")
	SkinInvalid           = define(GroupAccount, 14, http.StatusBadRequest, "Skins must be 64x64 or 64x32 PNG images.")
	CapeLocked            = define(GroupAccount, 15, http.StatusForbidden, "You have not unlocked this cape.")
	SkinUpdate            = define(GroupAccount, 16, http.StatusInternalServerError, "Could not save your skin.")
	SkinLoad              = define(GroupAccount, 17, http.StatusInternalServerError, "Could not load the skin.")
)

var (
	InstallDisabled  = define(GroupIdentity, 1, http.StatusForbidden, "This account is disabled.")
	AddressDisabled  = define(GroupIdentity, 2, http.StatusForbidden, "This account is disabled.")
	IdentityUpdate   = define(GroupIdentity, 3, http.StatusInternalServerError, "Could not update account information.")
	InstallNotFound  = define(GroupIdentity, 4, http.StatusNotFound, "Not found.")
	AddressNotFound  = define(GroupIdentity, 5, http.StatusNotFound, "Not found.")
	IdentityNotFound = define(GroupIdentity, 6, http.StatusNotFound, "Not found.")
)

var (
	UserNotFound       = define(GroupSocial, 1, http.StatusNotFound, "User not found.")
	RequestUserMissing = define(GroupSocial, 2, http.StatusNotFound, "User not found.")
	CannotAddSelf      = define(GroupSocial, 3, http.StatusBadRequest, "You cannot add yourself.")
	AlreadyFriends     = define(GroupSocial, 4, http.StatusConflict, "Already friends.")
	RequestNotFound    = define(GroupSocial, 5, http.StatusNotFound, "Friend request not found.")
	FriendsLoadFailed  = define(GroupSocial, 6, http.StatusInternalServerError, "Could not load friends.")
	RequestUpdate      = define(GroupSocial, 7, http.StatusInternalServerError, "Could not update friend request.")
	FriendsUpdate      = define(GroupSocial, 8, http.StatusInternalServerError, "Could not update friends.")
	PresenceNotFound   = define(GroupSocial, 9, http.StatusNotFound, "Presence not found.")
	PresenceUpdate     = define(GroupSocial, 10, http.StatusInternalServerError, "Could not update presence.")
	PresenceStore      = define(GroupSocial, 11, http.StatusInternalServerError, "Could not update presence.")
)

var (
	SessionNotFound    = define(GroupSession, 1, http.StatusNotFound, "Session not found.")
	NotSessionHost     = define(GroupSession, 2, http.StatusForbidden, "You are not the session host.")
	SessionNotJoinable = define(GroupSession, 3, http.StatusConflict, "Session is not joinable.")
	SessionCreate      = define(GroupSession, 4, http.StatusInternalServerError, "Could not create session.")
	SessionUpdate      = define(GroupSession, 5, http.StatusInternalServerError, "Could not update session.")
	SessionLoad        = define(GroupSession, 6, http.StatusInternalServerError, "Could not load sessions.")
	TicketNotFound     = define(GroupSession, 7, http.StatusNotFound, "Join request not found.")
	TicketCreate       = define(GroupSession, 8, http.StatusInternalServerError, "Could not create join request.")
	TicketNotYours     = define(GroupSession, 9, http.StatusForbidden, "You cannot use this join request.")
	PeerNotFound       = define(GroupSession, 10, http.StatusNotFound, "Player not found in this session.")
	PeerUpdate         = define(GroupSession, 11, http.StatusInternalServerError, "Could not update connection.")
	PeerLoad           = define(GroupSession, 12, http.StatusInternalServerError, "Could not load connections.")
	PeerInvalid        = define(GroupSession, 13, http.StatusBadRequest, "Invalid connection data.")
)

var (
	InviteNotFound = define(GroupInvite, 1, http.StatusNotFound, "Invite not found.")
	InviteUserGone = define(GroupInvite, 2, http.StatusNotFound, "User not found.")
	CannotInviteMe = define(GroupInvite, 3, http.StatusBadRequest, "You cannot invite yourself.")
	InviteUpdate   = define(GroupInvite, 4, http.StatusInternalServerError, "Could not update invite.")
	InviteLoad     = define(GroupInvite, 5, http.StatusInternalServerError, "Could not load invites.")
	InviteNotYours = define(GroupInvite, 6, http.StatusForbidden, "You cannot use this invite.")
	InviteExpired  = define(GroupInvite, 7, http.StatusGone, "This invite is no longer valid.")
)

var (
	ServerNotFound     = define(GroupServer, 1, http.StatusNotFound, "Server not found.")
	ServerNameInvalid  = define(GroupServer, 2, http.StatusBadRequest, "Server names are 3 to 24 letters, numbers, dashes or underscores.")
	ServerNameTaken    = define(GroupServer, 3, http.StatusConflict, "That server name is already taken.")
	ServerLimitReached = define(GroupServer, 4, http.StatusConflict, "You can only have 2 servers.")
	ServerTokenInvalid = define(GroupServer, 5, http.StatusUnauthorized, "The server token is invalid.")
	ServerUpdate       = define(GroupServer, 6, http.StatusInternalServerError, "Could not update the server.")
)

var (
	ForumNotFound     = define(GroupForum, 1, http.StatusNotFound, "That thread or post does not exist.")
	ForumTitleInvalid = define(GroupForum, 2, http.StatusBadRequest, "Titles are 3 to 100 characters.")
	ForumBodyInvalid  = define(GroupForum, 3, http.StatusBadRequest, "Posts are 1 to 10000 characters.")
	ForumLocked       = define(GroupForum, 4, http.StatusForbidden, "This thread is locked.")
	ForumForbidden    = define(GroupForum, 5, http.StatusForbidden, "You are not allowed to do that.")
	ForumUpdate       = define(GroupForum, 6, http.StatusInternalServerError, "Could not update the forum.")
	ForumMuted        = define(GroupForum, 7, http.StatusForbidden, "You are muted on the forum.")
	ForumAccountNew   = define(GroupForum, 8, http.StatusForbidden, "New accounts have to wait 10 minutes before posting.")
	ForumTooManyLinks = define(GroupForum, 9, http.StatusBadRequest, "New accounts can only post 2 links at a time.")
	ForumReportBad    = define(GroupForum, 10, http.StatusBadRequest, "Reports need a reason of 1 to 500 characters.")
	ForumDuplicateBad = define(GroupForum, 11, http.StatusBadRequest, "The original thread does not exist.")
)

var (
	AdminUpdate    = define(GroupAdmin, 1, http.StatusInternalServerError, "Could not update the account.")
	AdminProtected = define(GroupAdmin, 2, http.StatusForbidden, "This account cannot be changed this way.")
	AdminConfirm   = define(GroupAdmin, 3, http.StatusBadRequest, "Type the username exactly to confirm.")
	AdminInvalid   = define(GroupAdmin, 4, http.StatusBadRequest, "Invalid value.")
)

var (
	ClientConnect        = define(GroupClient, 1, 0, "Could not reach the Everstead servers.")
	ClientTimeout        = define(GroupClient, 2, 0, "The Everstead servers took too long to respond.")
	ClientBadResponse    = define(GroupClient, 3, 0, "The Everstead servers sent an unreadable response.")
	ClientSignedOut      = define(GroupClient, 4, 0, "You are not signed in.")
	ClientBadArgument    = define(GroupClient, 5, 0, "Invalid request.")
	ClientHostConnect    = define(GroupClient, 6, 0, "Could not connect to the host.")
	ClientHostTimeout    = define(GroupClient, 7, 0, "The host took too long to respond.")
	ClientConnectionLost = define(GroupClient, 8, 0, "The connection was lost.")
)

func Lookup(text string) (*Error, bool) {
	err, ok := registry[text]
	return err, ok
}

type Entry struct {
	Code    string `json:"code"`
	Message string `json:"message"`
	Group   uint16 `json:"group"`
}

func All() []Entry {
	entries := make([]Entry, 0, len(registry))
	for _, err := range registry {
		entries = append(entries, Entry{Code: err.Code.String(), Message: err.Message, Group: uint16(err.Code.Group)})
	}

	sort.Slice(entries, func(i, j int) bool { return entries[i].Code < entries[j].Code })
	return entries
}
