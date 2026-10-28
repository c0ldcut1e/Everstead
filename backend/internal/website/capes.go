package website

import (
	"slices"

	"github.com/everstead/backend/internal/store"
)

type cape struct {
	ID   string
	Name string
	File string
}

type capeView struct {
	cape
	Unlocked bool
	Selected bool
}

var capes = []cape{
	{"everstead:admin", "Admin", "admin"},
	{"everstead:early_player", "Early Player", "early_player"},
	{"everstead:bug_hunter_1", "Bug Hunter I", "bug_hunter_1"},
	{"everstead:bug_hunter_2", "Bug Hunter II", "bug_hunter_2"},
	{"everstead:bug_hunter_3", "Bug Hunter III", "bug_hunter_3"},
	{"everstead:bug_hunter_4", "Bug Hunter IV", "bug_hunter_4"},
	{"everstead:bug_hunter_5", "Bug Hunter V", "bug_hunter_5"},
	{"everstead:bug_hunter_6", "Bug Hunter VI", "bug_hunter_6"},
	{"everstead:bug_hunter_7", "Bug Hunter VII", "bug_hunter_7"},
	{"everstead:bug_hunter_8", "Bug Hunter VIII", "bug_hunter_8"},
	{"everstead:bug_hunter_9", "Bug Hunter IX", "bug_hunter_9"},
	{"everstead:bug_hunter_10", "Bug Hunter X", "bug_hunter_10"},
}

func capeViews(account *store.Account) []capeView {
	views := make([]capeView, len(capes))
	for i, entry := range capes {
		views[i] = capeView{cape: entry, Unlocked: slices.Contains(account.Capes, entry.ID), Selected: account.Cape == entry.ID}
	}

	return views
}
