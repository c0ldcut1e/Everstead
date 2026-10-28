package store

import (
	"context"
	"errors"
	"regexp"
	"time"

	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

type ForumThread struct {
	ThreadID    int64     `bson:"threadId"`
	Category    string    `bson:"category"`
	Title       string    `bson:"title"`
	AuthorPid   uint32    `bson:"authorPid"`
	Status      string    `bson:"status"`
	Locked      bool      `bson:"locked"`
	Pinned      bool      `bson:"pinned"`
	DuplicateOf int64     `bson:"duplicateOf"`
	ReplyCount  int       `bson:"replyCount"`
	VoteCount   int       `bson:"voteCount"`
	Deleted     bool      `bson:"deleted"`
	DeletedBy   uint32    `bson:"deletedBy"`
	DeletedAt   time.Time `bson:"deletedAt"`
	CreatedAt   time.Time `bson:"createdAt"`
	LastPostAt  time.Time `bson:"lastPostAt"`
}

type ForumPost struct {
	PostID    string    `bson:"postId"`
	ThreadID  int64     `bson:"threadId"`
	AuthorPid uint32    `bson:"authorPid"`
	Body      string    `bson:"body"`
	First     bool      `bson:"first"`
	Deleted   bool      `bson:"deleted"`
	DeletedBy uint32    `bson:"deletedBy"`
	DeletedAt time.Time `bson:"deletedAt"`
	EditedAt  time.Time `bson:"editedAt"`
	CreatedAt time.Time `bson:"createdAt"`
}

type ForumReport struct {
	ReportID    string    `bson:"reportId"`
	ReporterPid uint32    `bson:"reporterPid"`
	ThreadID    int64     `bson:"threadId"`
	PostID      string    `bson:"postId"`
	Reason      string    `bson:"reason"`
	Resolved    bool      `bson:"resolved"`
	ResolvedBy  uint32    `bson:"resolvedBy"`
	CreatedAt   time.Time `bson:"createdAt"`
}

type ModLogEntry struct {
	ActorPid  uint32    `bson:"actorPid"`
	Action    string    `bson:"action"`
	TargetPid uint32    `bson:"targetPid"`
	ThreadID  int64     `bson:"threadId"`
	PostID    string    `bson:"postId"`
	Detail    string    `bson:"detail"`
	CreatedAt time.Time `bson:"createdAt"`
}

type ForumUserStats struct {
	Threads int64
	Posts   int64
}

var notDeleted = bson.M{"$ne": true}

func (s *Store) CreateForumThread(ctx context.Context, thread ForumThread, body string) (*ForumThread, error) {
	threadID, err := s.nextNumber(ctx, "forumThread", 0)
	if err != nil {
		return nil, err
	}

	thread.ThreadID = threadID
	thread.CreatedAt = now()
	thread.LastPostAt = thread.CreatedAt
	if _, err := s.forumThreads().InsertOne(ctx, thread); err != nil {
		return nil, err
	}

	post := ForumPost{PostID: randomText(8), ThreadID: threadID, AuthorPid: thread.AuthorPid, Body: body, First: true, CreatedAt: thread.CreatedAt}
	if _, err := s.forumPosts().InsertOne(ctx, post); err != nil {
		return nil, err
	}

	return &thread, nil
}

func (s *Store) ForumThreadByID(ctx context.Context, threadID int64) (*ForumThread, error) {
	thread := &ForumThread{}
	err := s.forumThreads().FindOne(ctx, bson.M{"threadId": threadID}).Decode(thread)
	if errors.Is(err, mongo.ErrNoDocuments) {
		return nil, ErrNotFound
	}

	if err != nil {
		return nil, err
	}

	return thread, nil
}

func (s *Store) findForumThreads(ctx context.Context, filter bson.M, sort bson.D, skip int64, limit int64) ([]ForumThread, int64, error) {
	total, err := s.forumThreads().CountDocuments(ctx, filter)
	if err != nil {
		return nil, 0, err
	}

	threads := []ForumThread{}
	cursor, err := s.forumThreads().Find(ctx, filter, options.Find().SetSort(sort).SetSkip(skip).SetLimit(limit))
	if err != nil {
		return nil, 0, err
	}

	defer cursor.Close(ctx)

	if err := cursor.All(ctx, &threads); err != nil {
		return nil, 0, err
	}

	return threads, total, nil
}

func (s *Store) ForumThreads(ctx context.Context, category string, top bool, skip int64, limit int64) ([]ForumThread, int64, error) {
	sort := bson.D{{Key: "pinned", Value: -1}, {Key: "lastPostAt", Value: -1}}
	if top {
		sort = bson.D{{Key: "pinned", Value: -1}, {Key: "voteCount", Value: -1}, {Key: "lastPostAt", Value: -1}}
	}

	return s.findForumThreads(ctx, bson.M{"category": category, "deleted": notDeleted}, sort, skip, limit)
}

func (s *Store) LatestForumThread(ctx context.Context, category string) (*ForumThread, int64, error) {
	threads, total, err := s.findForumThreads(ctx, bson.M{"category": category, "deleted": notDeleted},
		bson.D{{Key: "lastPostAt", Value: -1}}, 0, 1)
	if err != nil || len(threads) == 0 {
		return nil, total, err
	}

	return &threads[0], total, nil
}

func (s *Store) ForumThreadsByAuthor(ctx context.Context, authorPid uint32, limit int64) ([]ForumThread, error) {
	threads, _, err := s.findForumThreads(ctx, bson.M{"authorPid": authorPid, "deleted": notDeleted},
		bson.D{{Key: "createdAt", Value: -1}}, 0, limit)
	return threads, err
}

// SearchForum matches thread titles and post bodies, and only returns threads that are not deleted
func (s *Store) SearchForum(ctx context.Context, query string, limit int64) ([]ForumThread, error) {
	pattern := bson.M{"$regex": regexp.QuoteMeta(query), "$options": "i"}
	threadIDs, err := s.forumPosts().Distinct(ctx, "threadId", bson.M{"body": pattern, "deleted": notDeleted})
	if err != nil {
		return nil, err
	}

	filter := bson.M{"deleted": notDeleted, "$or": []bson.M{{"title": pattern}, {"threadId": bson.M{"$in": threadIDs}}}}
	threads, _, err := s.findForumThreads(ctx, filter, bson.D{{Key: "lastPostAt", Value: -1}}, 0, limit)
	return threads, err
}

func (s *Store) ForumUserStats(ctx context.Context, pid uint32) (ForumUserStats, error) {
	threads, err := s.forumThreads().CountDocuments(ctx, bson.M{"authorPid": pid, "deleted": notDeleted})
	if err != nil {
		return ForumUserStats{}, err
	}

	posts, err := s.forumPosts().CountDocuments(ctx, bson.M{"authorPid": pid, "deleted": notDeleted})
	return ForumUserStats{Threads: threads, Posts: posts}, err
}

func (s *Store) ForumPosts(ctx context.Context, threadID int64) ([]ForumPost, error) {
	posts := []ForumPost{}
	cursor, err := s.forumPosts().Find(ctx, bson.M{"threadId": threadID}, options.Find().SetSort(bson.D{{Key: "createdAt", Value: 1}}))
	if err != nil {
		return nil, err
	}

	defer cursor.Close(ctx)

	if err := cursor.All(ctx, &posts); err != nil {
		return nil, err
	}

	return posts, nil
}

func (s *Store) ForumPostByID(ctx context.Context, postID string) (*ForumPost, error) {
	post := &ForumPost{}
	err := s.forumPosts().FindOne(ctx, bson.M{"postId": postID}).Decode(post)
	if errors.Is(err, mongo.ErrNoDocuments) {
		return nil, ErrNotFound
	}

	if err != nil {
		return nil, err
	}

	return post, nil
}

func (s *Store) CreateForumPost(ctx context.Context, threadID int64, authorPid uint32, body string) error {
	post := ForumPost{PostID: randomText(8), ThreadID: threadID, AuthorPid: authorPid, Body: body, CreatedAt: now()}
	if _, err := s.forumPosts().InsertOne(ctx, post); err != nil {
		return err
	}

	_, err := s.forumThreads().UpdateOne(ctx, bson.M{"threadId": threadID},
		bson.M{"$set": bson.M{"lastPostAt": post.CreatedAt}, "$inc": bson.M{"replyCount": 1}})
	return err
}

func (s *Store) EditForumPost(ctx context.Context, postID string, body string) error {
	_, err := s.forumPosts().UpdateOne(ctx, bson.M{"postId": postID}, bson.M{"$set": bson.M{"body": body, "editedAt": now()}})
	return err
}

func (s *Store) SetForumPostDeleted(ctx context.Context, post *ForumPost, deleted bool, actorPid uint32) error {
	result, err := s.forumPosts().UpdateOne(ctx, bson.M{"postId": post.PostID, "deleted": bson.M{"$ne": deleted}},
		bson.M{"$set": bson.M{"deleted": deleted, "deletedBy": actorPid, "deletedAt": now()}})
	if err != nil || result.ModifiedCount == 0 {
		return err
	}

	change := -1
	if !deleted {
		change = 1
	}

	_, err = s.forumThreads().UpdateOne(ctx, bson.M{"threadId": post.ThreadID}, bson.M{"$inc": bson.M{"replyCount": change}})
	return err
}

// DeleteForumPostsByAuthor soft deletes replies only, since a deleted first post would leave its thread without a body
func (s *Store) DeleteForumPostsByAuthor(ctx context.Context, authorPid uint32, actorPid uint32) (int, error) {
	posts := []ForumPost{}
	cursor, err := s.forumPosts().Find(ctx, bson.M{"authorPid": authorPid, "first": bson.M{"$ne": true}, "deleted": notDeleted})
	if err != nil {
		return 0, err
	}

	defer cursor.Close(ctx)

	if err := cursor.All(ctx, &posts); err != nil {
		return 0, err
	}

	for i := range posts {
		if err := s.SetForumPostDeleted(ctx, &posts[i], true, actorPid); err != nil {
			return i, err
		}
	}

	_, err = s.forumThreads().UpdateMany(ctx, bson.M{"authorPid": authorPid, "deleted": notDeleted},
		bson.M{"$set": bson.M{"deleted": true, "deletedBy": actorPid, "deletedAt": now()}})
	return len(posts), err
}

func (s *Store) UpdateForumThread(ctx context.Context, threadID int64, fields bson.M) error {
	_, err := s.forumThreads().UpdateOne(ctx, bson.M{"threadId": threadID}, bson.M{"$set": fields})
	return err
}

func (s *Store) SetForumThreadDeleted(ctx context.Context, threadID int64, deleted bool, actorPid uint32) error {
	return s.UpdateForumThread(ctx, threadID, bson.M{"deleted": deleted, "deletedBy": actorPid, "deletedAt": now()})
}

func (s *Store) ToggleForumVote(ctx context.Context, threadID int64, pid uint32) error {
	result, err := s.forumVotes().DeleteOne(ctx, bson.M{"threadId": threadID, "pid": pid})
	if err != nil {
		return err
	}

	change := -1
	if result.DeletedCount == 0 {
		_, err := s.forumVotes().InsertOne(ctx, bson.M{"threadId": threadID, "pid": pid, "createdAt": now()})
		if mongo.IsDuplicateKeyError(err) {
			return nil
		}

		if err != nil {
			return err
		}

		change = 1
	}

	return s.forumThreads().FindOneAndUpdate(ctx, bson.M{"threadId": threadID}, bson.M{"$inc": bson.M{"voteCount": change}}).Err()
}

func (s *Store) HasForumVote(ctx context.Context, threadID int64, pid uint32) (bool, error) {
	count, err := s.forumVotes().CountDocuments(ctx, bson.M{"threadId": threadID, "pid": pid})
	return count > 0, err
}

func (s *Store) CreateForumReport(ctx context.Context, report ForumReport) error {
	report.ReportID = randomText(8)
	report.CreatedAt = now()
	_, err := s.forumReports().InsertOne(ctx, report)
	return err
}

func (s *Store) OpenForumReports(ctx context.Context) ([]ForumReport, error) {
	reports := []ForumReport{}
	cursor, err := s.forumReports().Find(ctx, bson.M{"resolved": false}, options.Find().SetSort(bson.D{{Key: "createdAt", Value: 1}}).SetLimit(100))
	if err != nil {
		return nil, err
	}

	defer cursor.Close(ctx)

	if err := cursor.All(ctx, &reports); err != nil {
		return nil, err
	}

	return reports, nil
}

func (s *Store) ResolveForumReport(ctx context.Context, reportID string, actorPid uint32) (*ForumReport, error) {
	report := &ForumReport{}
	err := s.forumReports().FindOneAndUpdate(ctx, bson.M{"reportId": reportID, "resolved": false},
		bson.M{"$set": bson.M{"resolved": true, "resolvedBy": actorPid}}).Decode(report)
	if errors.Is(err, mongo.ErrNoDocuments) {
		return nil, ErrNotFound
	}

	return report, err
}

func (s *Store) AddModLog(ctx context.Context, entry ModLogEntry) error {
	entry.CreatedAt = now()
	_, err := s.modLog().InsertOne(ctx, entry)
	return err
}

func (s *Store) ModLog(ctx context.Context, targetPid uint32, limit int64) ([]ModLogEntry, error) {
	filter := bson.M{}
	if targetPid != 0 {
		filter["targetPid"] = targetPid
	}

	entries := []ModLogEntry{}
	cursor, err := s.modLog().Find(ctx, filter, options.Find().SetSort(bson.D{{Key: "createdAt", Value: -1}}).SetLimit(limit))
	if err != nil {
		return nil, err
	}

	defer cursor.Close(ctx)

	if err := cursor.All(ctx, &entries); err != nil {
		return nil, err
	}

	return entries, nil
}
