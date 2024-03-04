#include <stdint.h>

#define MAX_REF_LEN 256

#define GIT_OID_SHA1_SIZE       20
#define GIT_OID_SHA1_HEXSIZE    (GIT_OID_SHA1_SIZE * 2)
#define GIT_OID_MAX_SIZE        GIT_OID_SHA1_SIZE
#define GIT_OID_MAX_HEXSIZE     GIT_OID_SHA1_HEXSIZE

#define GIT_CHECKOUT_OPTIONS_VERSION 1
#define GIT_CHECKOUT_OPTIONS_INIT {GIT_CHECKOUT_OPTIONS_VERSION, GIT_CHECKOUT_SAFE}

#define GIT_CLONE_OPTIONS_VERSION 1
#define GIT_CLONE_OPTIONS_INIT { GIT_CLONE_OPTIONS_VERSION, \
	{ GIT_CHECKOUT_OPTIONS_VERSION, GIT_CHECKOUT_SAFE }, \
	GIT_FETCH_OPTIONS_INIT }

#define GIT_FETCH_OPTIONS_VERSION 1
#define GIT_FETCH_OPTIONS_INIT { \
	GIT_FETCH_OPTIONS_VERSION, \
	GIT_REMOTE_CALLBACKS_INIT, \
	GIT_FETCH_PRUNE_UNSPECIFIED, \
	GIT_REMOTE_UPDATE_FETCHHEAD, \
	GIT_REMOTE_DOWNLOAD_TAGS_UNSPECIFIED, \
	GIT_PROXY_OPTIONS_INIT }

#define GIT_PROXY_OPTIONS_VERSION 1
#define GIT_PROXY_OPTIONS_INIT {GIT_PROXY_OPTIONS_VERSION}

#define GIT_REMOTE_CALLBACKS_VERSION 1
#define GIT_REMOTE_CALLBACKS_INIT {GIT_REMOTE_CALLBACKS_VERSION}

typedef enum
{
	GIT_OPT_GET_MWINDOW_SIZE,
	GIT_OPT_SET_MWINDOW_SIZE,
	GIT_OPT_GET_MWINDOW_MAPPED_LIMIT,
	GIT_OPT_SET_MWINDOW_MAPPED_LIMIT,
	GIT_OPT_GET_SEARCH_PATH,
	GIT_OPT_SET_SEARCH_PATH,
	GIT_OPT_SET_CACHE_OBJECT_LIMIT,
	GIT_OPT_SET_CACHE_MAX_SIZE,
	GIT_OPT_ENABLE_CACHING,
	GIT_OPT_GET_CACHED_MEMORY,
	GIT_OPT_GET_TEMPLATE_PATH,
	GIT_OPT_SET_TEMPLATE_PATH,
	GIT_OPT_SET_SSL_CERT_LOCATIONS,
	GIT_OPT_SET_USER_AGENT,
	GIT_OPT_ENABLE_STRICT_OBJECT_CREATION,
	GIT_OPT_ENABLE_STRICT_SYMBOLIC_REF_CREATION,
	GIT_OPT_SET_SSL_CIPHERS,
	GIT_OPT_GET_USER_AGENT,
	GIT_OPT_ENABLE_OFS_DELTA,
	GIT_OPT_ENABLE_FSYNC_GITDIR,
	GIT_OPT_GET_WINDOWS_SHAREMODE,
	GIT_OPT_SET_WINDOWS_SHAREMODE,
	GIT_OPT_ENABLE_STRICT_HASH_VERIFICATION,
	GIT_OPT_SET_ALLOCATOR,
	GIT_OPT_ENABLE_UNSAVED_INDEX_SAFETY,
	GIT_OPT_GET_PACK_MAX_OBJECTS,
	GIT_OPT_SET_PACK_MAX_OBJECTS,
	GIT_OPT_DISABLE_PACK_KEEP_FILE_CHECKS,
	GIT_OPT_ENABLE_HTTP_EXPECT_CONTINUE,
	GIT_OPT_GET_MWINDOW_FILE_LIMIT,
	GIT_OPT_SET_MWINDOW_FILE_LIMIT,
	GIT_OPT_SET_ODB_PACKED_PRIORITY,
	GIT_OPT_SET_ODB_LOOSE_PRIORITY,
	GIT_OPT_GET_EXTENSIONS,
	GIT_OPT_SET_EXTENSIONS,
	GIT_OPT_GET_OWNER_VALIDATION,
	GIT_OPT_SET_OWNER_VALIDATION,
	GIT_OPT_GET_HOMEDIR,
	GIT_OPT_SET_HOMEDIR,
	GIT_OPT_SET_SERVER_CONNECT_TIMEOUT,
	GIT_OPT_GET_SERVER_CONNECT_TIMEOUT,
	GIT_OPT_SET_SERVER_TIMEOUT,
	GIT_OPT_GET_SERVER_TIMEOUT
} git_libgit2_opt_t;

typedef enum
{
	GIT_BRANCH_LOCAL = 1,
	GIT_BRANCH_REMOTE = 2,
	GIT_BRANCH_ALL = GIT_BRANCH_LOCAL|GIT_BRANCH_REMOTE
} git_branch_t;

typedef enum
{
	GIT_CLONE_LOCAL_AUTO,
	GIT_CLONE_LOCAL,
	GIT_CLONE_NO_LOCAL,
	GIT_CLONE_LOCAL_NO_LINKS
} git_clone_local_t;

typedef enum
{
	GIT_CHECKOUT_NOTIFY_NONE      = 0,
	GIT_CHECKOUT_NOTIFY_CONFLICT  = (1u << 0),
	GIT_CHECKOUT_NOTIFY_DIRTY     = (1u << 1),
	GIT_CHECKOUT_NOTIFY_UPDATED   = (1u << 2),
	GIT_CHECKOUT_NOTIFY_UNTRACKED = (1u << 3),
	GIT_CHECKOUT_NOTIFY_IGNORED   = (1u << 4),
	GIT_CHECKOUT_NOTIFY_ALL       = 0x0FFFFu
} git_checkout_notify_t;

typedef enum
{
	GIT_CHECKOUT_NONE = 0,
	GIT_CHECKOUT_SAFE = (1u << 0),
	GIT_CHECKOUT_FORCE = (1u << 1),
	GIT_CHECKOUT_RECREATE_MISSING = (1u << 2),
	GIT_CHECKOUT_ALLOW_CONFLICTS = (1u << 4),
	GIT_CHECKOUT_REMOVE_UNTRACKED = (1u << 5),
	GIT_CHECKOUT_REMOVE_IGNORED = (1u << 6),
	GIT_CHECKOUT_UPDATE_ONLY = (1u << 7),
	GIT_CHECKOUT_DONT_UPDATE_INDEX = (1u << 8),
	GIT_CHECKOUT_NO_REFRESH = (1u << 9),
	GIT_CHECKOUT_SKIP_UNMERGED = (1u << 10),
	GIT_CHECKOUT_USE_OURS = (1u << 11),
	GIT_CHECKOUT_USE_THEIRS = (1u << 12),
	GIT_CHECKOUT_DISABLE_PATHSPEC_MATCH = (1u << 13),
	GIT_CHECKOUT_SKIP_LOCKED_DIRECTORIES = (1u << 18),
	GIT_CHECKOUT_DONT_OVERWRITE_IGNORED = (1u << 19),
	GIT_CHECKOUT_CONFLICT_STYLE_MERGE = (1u << 20),
	GIT_CHECKOUT_CONFLICT_STYLE_DIFF3 = (1u << 21),
	GIT_CHECKOUT_DONT_REMOVE_EXISTING = (1u << 22),
	GIT_CHECKOUT_DONT_WRITE_INDEX = (1u << 23),
	GIT_CHECKOUT_DRY_RUN = (1u << 24),
	GIT_CHECKOUT_CONFLICT_STYLE_ZDIFF3 = (1u << 25),
	GIT_CHECKOUT_UPDATE_SUBMODULES = (1u << 16),
	GIT_CHECKOUT_UPDATE_SUBMODULES_IF_CHANGED = (1u << 17)
} git_checkout_strategy_t;

typedef enum
{
	GIT_DELTA_UNMODIFIED = 0,
	GIT_DELTA_ADDED = 1,
	GIT_DELTA_DELETED = 2,
	GIT_DELTA_MODIFIED = 3,
	GIT_DELTA_RENAMED = 4,
	GIT_DELTA_COPIED = 5,
	GIT_DELTA_IGNORED = 6,
	GIT_DELTA_UNTRACKED = 7,
	GIT_DELTA_TYPECHANGE = 8,
	GIT_DELTA_UNREADABLE = 9,
	GIT_DELTA_CONFLICTED = 10
} git_delta_t;

typedef enum
{
	GIT_ERROR_NONE = 0,
	GIT_ERROR_NOMEMORY,
	GIT_ERROR_OS,
	GIT_ERROR_INVALID,
	GIT_ERROR_REFERENCE,
	GIT_ERROR_ZLIB,
	GIT_ERROR_REPOSITORY,
	GIT_ERROR_CONFIG,
	GIT_ERROR_REGEX,
	GIT_ERROR_ODB,
	GIT_ERROR_INDEX,
	GIT_ERROR_OBJECT,
	GIT_ERROR_NET,
	GIT_ERROR_TAG,
	GIT_ERROR_TREE,
	GIT_ERROR_INDEXER,
	GIT_ERROR_SSL,
	GIT_ERROR_SUBMODULE,
	GIT_ERROR_THREAD,
	GIT_ERROR_STASH,
	GIT_ERROR_CHECKOUT,
	GIT_ERROR_FETCHHEAD,
	GIT_ERROR_MERGE,
	GIT_ERROR_SSH,
	GIT_ERROR_FILTER,
	GIT_ERROR_REVERT,
	GIT_ERROR_CALLBACK,
	GIT_ERROR_CHERRYPICK,
	GIT_ERROR_DESCRIBE,
	GIT_ERROR_REBASE,
	GIT_ERROR_FILESYSTEM,
	GIT_ERROR_PATCH,
	GIT_ERROR_WORKTREE,
	GIT_ERROR_SHA,
	GIT_ERROR_HTTP,
	GIT_ERROR_INTERNAL,
	GIT_ERROR_GRAFTS
} git_error_t;

typedef enum
{
	GIT_FETCH_PRUNE_UNSPECIFIED,
	GIT_FETCH_PRUNE,
	GIT_FETCH_NO_PRUNE
} git_fetch_prune_t;

typedef enum
{
	GIT_OBJECT_ANY = -2,
	GIT_OBJECT_INVALID = -1,
	GIT_OBJECT_COMMIT = 1,
	GIT_OBJECT_TREE = 2,
	GIT_OBJECT_BLOB = 3,
	GIT_OBJECT_TAG = 4,
	GIT_OBJECT_OFS_DELTA = 6,
	GIT_OBJECT_REF_DELTA = 7
} git_object_t;

typedef enum
{
	GIT_OID_SHA1 = 1,
	GIT_OID_SHA256 = 2
} git_oid_t;

typedef enum
{
	GIT_PROXY_NONE,
	GIT_PROXY_AUTO,
	GIT_PROXY_SPECIFIED
} git_proxy_t;

typedef enum
{
	GIT_REMOTE_DOWNLOAD_TAGS_UNSPECIFIED = 0,
	GIT_REMOTE_DOWNLOAD_TAGS_AUTO,
	GIT_REMOTE_DOWNLOAD_TAGS_NONE,
	GIT_REMOTE_DOWNLOAD_TAGS_ALL
} git_remote_autotag_option_t;

typedef enum
{
	GIT_REMOTE_COMPLETION_DOWNLOAD,
	GIT_REMOTE_COMPLETION_INDEXING,
	GIT_REMOTE_COMPLETION_ERROR
} git_remote_completion_t;

typedef enum
{
	GIT_REMOTE_REDIRECT_NONE = (1 << 0),
	GIT_REMOTE_REDIRECT_INITIAL = (1 << 1),
	GIT_REMOTE_REDIRECT_ALL = (1 << 2)
} git_remote_redirect_t;

typedef enum
{
	GIT_REMOTE_UPDATE_FETCHHEAD = (1 << 0),
	GIT_REMOTE_UPDATE_REPORT_UNCHANGED = (1 << 1)
} git_remote_update_flags;

typedef enum
{
	GIT_RESET_SOFT = 1,
	GIT_RESET_MIXED = 2,
	GIT_RESET_HARD = 3
} git_reset_t;


typedef enum
{
	GIT_SORT_NONE = 0,
	GIT_SORT_TOPOLOGICAL = 1 << 0,
	GIT_SORT_TIME = 1 << 1,
	GIT_SORT_REVERSE = 1 << 2,
} git_sort_t;

typedef enum
{
	GIT_SUBMODULE_IGNORE_UNSPECIFIED  = -1,
	GIT_SUBMODULE_IGNORE_NONE = 1,
	GIT_SUBMODULE_IGNORE_UNTRACKED = 2,
	GIT_SUBMODULE_IGNORE_DIRTY = 3,
	GIT_SUBMODULE_IGNORE_ALL = 4
} git_submodule_ignore_t;

typedef enum
{
	GIT_TREEWALK_PRE = 0,
	GIT_TREEWALK_POST = 1
} git_treewalk_mode;

typedef int64_t git_off_t;
typedef int64_t git_time_t;
typedef uint64_t git_object_size_t;
typedef struct git_blob git_blob;
typedef struct git_branch_iterator git_branch_iterator;
typedef struct git_cert git_cert;
typedef struct git_commit git_commit;
typedef struct git_credential git_credential;
typedef struct git_diff git_diff;
typedef struct git_index git_index;
typedef struct git_object git_object;
typedef struct git_reference git_reference;
typedef struct git_remote git_remote;
typedef struct git_repository git_repository;
typedef struct git_revwalk git_revwalk;
typedef struct git_transport git_transport;
typedef struct git_tree git_tree;
typedef struct git_tree_entry git_tree_entry;
typedef struct git_treebuilder git_treebuilder;
typedef struct git_buf git_buf;
typedef struct git_diff_delta git_diff_delta;
typedef struct git_diff_file git_diff_file;
typedef struct git_checkout_perfdata git_checkout_perfdata;
typedef struct git_indexer_progress git_indexer_progress;
typedef struct git_push_update git_push_update;

typedef int (*git_checkout_notify_cb)(git_checkout_notify_t why, const char *path, const git_diff_file *baseline, const git_diff_file *target, const git_diff_file *workdir, void *payload);
typedef int (*git_credential_acquire_cb)(git_credential **out, const char *url, const char *username_from_url, unsigned int allowed_types, void *payload);
typedef int (*git_diff_notify_cb)(const git_diff *diff_so_far, const git_diff_delta *delta_to_add, const char *matched_pathspec, void *payload);
typedef int (*git_diff_progress_cb)(const git_diff *diff_so_far, const char *old_path, const char *new_path, void *payload);
typedef int (*git_indexer_progress_cb)(const git_indexer_progress *stats, void *payload);
typedef int (*git_packbuilder_progress)(int stage, uint32_t current, uint32_t total, void *payload);
typedef int (*git_push_negotiation)(const git_push_update **updates, size_t len, void *payload);
typedef int (*git_push_transfer_progress_cb)(unsigned int current, unsigned int total, size_t bytes, void *payload);
typedef int (*git_push_update_reference_cb)(const char *refname, const char *status, void *data);
typedef int (*git_repository_create_cb)(git_repository **out, const char *path, int bare, void *payload);
typedef int (*git_remote_create_cb)(git_remote **out, git_repository *repo, const char *name, const char *url, void *payload);
typedef int (*git_remote_ready_cb)(git_remote *remote, int direction, void *payload);
typedef int (*git_transport_cb)(git_transport **out, git_remote *owner, void *param);
typedef int (*git_transport_certificate_check_cb)(git_cert *cert, int valid, const char *host, void *payload);
typedef int (*git_transport_message_cb)(const char *str, int len, void *payload);
typedef int (*git_treewalk_cb)(const char *root, const git_tree_entry *entry, void *payload);
typedef int (*git_url_resolve_cb)(git_buf *url_resolved, const char *url, int direction, void *payload);
typedef void (*git_checkout_perfdata_cb)(const git_checkout_perfdata *perfdata, void *payload);
typedef void (*git_checkout_progress_cb)(const char *path, size_t completed_steps, size_t total_steps, void *payload);

typedef struct git_time
{
	git_time_t time;
	int offset;
	char sign;
} git_time;

typedef struct git_buf
{
	char *ptr;
	size_t reserved;
	size_t size;
} git_buf;

typedef struct git_oid
{
	unsigned char id[GIT_OID_MAX_SIZE];
} git_oid;

typedef struct git_signature
{
	char *name;
	char *email;
	git_time when;
} git_signature;

typedef struct git_push_update
{
	char *src_refname;
	char *dst_refname;
	git_oid src;
	git_oid dst;
} git_push_update;

typedef struct git_checkout_perfdata
{
	size_t mkdir_calls;
	size_t stat_calls;
	size_t chmod_calls;
} git_checkout_perfdata;

typedef struct git_strarray
{
	char **strings;
	size_t count;
} git_strarray;

typedef struct git_diff_file
{
	git_oid id;
	const char *path;
	git_object_size_t size;
	uint32_t flags;
	uint16_t mode;
	uint16_t id_abbrev;
} git_diff_file;

typedef struct git_diff_delta
{
	git_delta_t status;
	uint32_t flags;
	uint16_t similarity;
	uint16_t nfiles;
	git_diff_file old_file;
	git_diff_file new_file;
} git_diff_delta;

typedef struct git_indexer_progress
{
	unsigned int total_objects;
	unsigned int indexed_objects;
	unsigned int received_objects;
	unsigned int local_objects;
	unsigned int total_deltas;
	unsigned int indexed_deltas;
	size_t received_bytes;
} git_indexer_progress;

typedef struct git_remote_callbacks
{
	unsigned int version;
	git_transport_message_cb sideband_progress;
	int (*completion)(git_remote_completion_t type, void *data);
	git_credential_acquire_cb credentials;
	git_transport_certificate_check_cb certificate_check;
	git_indexer_progress_cb transfer_progress;
	int (*update_tips)(const char *refname, const git_oid *a, const git_oid *b, void *data);
	git_packbuilder_progress pack_progress;
	git_push_transfer_progress_cb push_transfer_progress;
	git_push_update_reference_cb push_update_reference;
	git_push_negotiation push_negotiation;
	git_transport_cb transport;
	git_remote_ready_cb remote_ready;
	void *payload;
	git_url_resolve_cb resolve_url;
} git_remote_callbacks;

typedef struct git_proxy_options
{
	unsigned int version;
	git_proxy_t type;
	const char *url;
	git_credential_acquire_cb credentials;
	git_transport_certificate_check_cb certificate_check;
	void *payload;
} git_proxy_options;

typedef struct git_checkout_options
{
	unsigned int version;
	unsigned int checkout_strategy;
	int disable_filters;
	unsigned int dir_mode;
	unsigned int file_mode;
	int file_open_flags;
	unsigned int notify_flags;
	git_checkout_notify_cb notify_cb;
	void *notify_payload;
	git_checkout_progress_cb progress_cb;
	void *progress_payload;
	git_strarray paths;
	git_tree *baseline;
	git_index *baseline_index;
	const char *target_directory;
	const char *ancestor_label;
	const char *our_label;
	const char *their_label;
	git_checkout_perfdata_cb perfdata_cb;
	void *perfdata_payload;
} git_checkout_options;

typedef struct git_fetch_options
{
	int version;
	git_remote_callbacks callbacks;
	git_fetch_prune_t prune;
	unsigned int update_flags;
	git_remote_autotag_option_t download_tags;
	git_proxy_options proxy_opts;
	int depth;
	git_remote_redirect_t follow_redirects;
	git_strarray custom_headers;
} git_fetch_options;

typedef struct git_clone_options
{
    unsigned int version;
    git_checkout_options checkout_opts;
    git_fetch_options fetch_opts;
    int bare;
    git_clone_local_t local;
    const char *checkout_branch;
    git_repository_create_cb repository_cb;
    void *repository_cb_payload;
    git_remote_create_cb remote_cb;
    void *remote_cb_payload;
} git_clone_options;

typedef struct git_diff_options
{
	unsigned int version;
	uint32_t flags;
	git_submodule_ignore_t ignore_submodules;
	git_strarray pathspec;
	git_diff_notify_cb notify_cb;
	git_diff_progress_cb progress_cb;
	void *payload;
	uint32_t context_lines;
	uint32_t interhunk_lines;
	git_oid_t oid_type;
	uint16_t id_abbrev;
	git_off_t max_size;
	const char *old_prefix;
	const char *new_prefix;
} git_diff_options;

typedef struct git_push_options
{
	unsigned int version;
	unsigned int pb_parallelism;
	git_remote_callbacks callbacks;
	git_proxy_options proxy_opts;
	git_remote_redirect_t follow_redirects;
	git_strarray custom_headers;
} git_push_options;

typedef struct git_error
{
	char *message;
	int klass;
    int padding;
} git_error;

typedef char *GitOidToStr(char *out, size_t n, const git_oid *id);
typedef const char *GitCommitMessage(const git_commit *commit);
typedef const char *GitReferenceName(const git_reference *ref);
typedef const git_error *GitErrorLast(void);
typedef const git_oid *GitCommitId(const git_commit *commit);
typedef const git_oid *GitTreeEntryId(const git_tree_entry *entry);
typedef const git_signature *GitCommitAuthor(const git_commit *commit);
typedef const git_signature *GitCommitCommitter(const git_commit *commit);
typedef const void *GitBlobRawContent(const git_blob *blob);
typedef git_object_size_t GitBlobRawSize(const git_blob *blob);
typedef git_object_t GitTreeEntryType(const git_tree_entry *entry);
typedef int GitBlobCreateFromBuffer(git_oid *id, git_repository *repo, const void *buffer, size_t len);
typedef int GitBlobLookup(git_blob **blob, git_repository *repo, const git_oid *id);
typedef int GitBranchCreate(git_reference **out, git_repository *repo, const char *branch_name, const git_commit *target, int force);
typedef int GitBranchDelete(git_reference *branch);
typedef int GitBranchIteratorNew(git_branch_iterator **out, git_repository *repo, git_branch_t list_flags);
typedef int GitBranchName(const char **out, const git_reference *ref);
typedef int GitBranchNext(git_reference **out, git_branch_t *out_type, git_branch_iterator *iter);
typedef int GitBranchSetUpstream(git_reference *branch, const char *branch_name);
typedef int GitBranchUpstream(git_reference **out, const git_reference *branch);
typedef int GitClone(git_repository **out, const char *url, const char *local_path, const git_clone_options *option);
typedef int GitCommitCreate(git_oid *id, git_repository *repo, const char *update_ref, const git_signature *author, const git_signature *committer, const char *message_encoding, const char *message, const git_tree *tree, size_t parent_count, const git_commit *parents[]);
typedef int GitCommitLookup(git_commit **commit, git_repository *repo, const git_oid *id);
typedef int GitCommitParent(git_commit **out, const git_commit *commit, unsigned int n);
typedef int GitCommitTree(git_tree **tree_out, const git_commit *commit);
typedef int GitDiffTreeToTree(git_diff **diff, git_repository *repo, git_tree *old_tree, git_tree *new_tree, const git_diff_options *opts);
typedef int GitLibgit2Init(void);
typedef int	GitLibgit2Opts(int option, ...);
typedef int GitLibgit2Shutdown(void);
typedef int GitOidCmp(const git_oid *a, const git_oid *b);
typedef int GitOidFromStrN(git_oid *out, const char *str, size_t length);
typedef int GitReferenceLookup(git_reference **out, git_repository *repo, const char *name);
typedef int GitReferenceNameToId(git_oid *out, git_repository *repo, const char *name);
typedef int GitRemoteCreate(git_remote **out, git_repository *repo, const char *name, const char *url);
typedef int GitRemoteFetch(git_remote *remote, const git_strarray *refspecs, const git_fetch_options *opts, const char *reflog_message);
typedef int GitRemoteLookup(git_remote **out, git_repository *repo, const char *name);
typedef int GitRemotePrune(git_remote *remote, const git_remote_callbacks *callbacks);
typedef int GitRemotePush(git_remote *remote, const git_strarray *refspecs, const git_push_options *opts);
typedef int GitRepositoryOpen(git_repository **out, const char *path);
typedef int GitReset(git_repository *repo, const git_object *target, git_reset_t reset_type, const git_checkout_options *checkout_opts);
typedef int GitRevwalkHide(git_revwalk *walk, const git_oid *commit_id);
typedef int GitRevwalkNew(git_revwalk **out, git_repository *repo);
typedef int GitRevwalkNext(git_oid *out, git_revwalk *walk);
typedef int GitRevwalkPush(git_revwalk *walk, const git_oid *id);
typedef int GitRevwalkPushHead(git_revwalk *walk);
typedef int GitRevwalkSorting(git_revwalk *walk, unsigned int sort_mode);
typedef int GitSignatureNew(git_signature **out, const char *name, const char *email, git_time_t time, int offset);
typedef int GitTreeLookup(git_tree **out, git_repository *repo, const git_oid *id);
typedef int GitTreeWalk(const git_tree *tree, git_treewalk_mode mode, git_treewalk_cb callback, void *payload);
typedef int GitTreebuilderNew(git_treebuilder **out, git_repository *repo, const git_tree *source);
typedef int GitTreebuilderWrite(git_oid *id, git_treebuilder *bld);
typedef size_t GitDiffNumDeltas(const git_diff *diff);
typedef unsigned int GitCommitParentCount(const git_commit *commit);
typedef void GitBranchIteratorFree(git_branch_iterator *iter);
typedef void GitCommitFree(git_commit *commit);
typedef void GitDiffFree(git_diff *diff);
typedef void GitReferenceFree(git_reference *ref);
typedef void GitRemoteFree(git_remote *remote);
typedef void GitRepositoryFree(git_repository *repo);
typedef void GitRevwalkFree(git_revwalk *walk);
typedef void GitSignatureFree(git_signature *sig);
typedef void GitTreeFree(git_tree *tree);
typedef void GitTreebuilderFree(git_treebuilder *bld);

typedef struct Git2Code
{
    GitBlobCreateFromBuffer *git_blob_create_from_buffer;
    GitBlobLookup *git_blob_lookup;
    GitBlobRawContent *git_blob_rawcontent;
    GitBlobRawSize *git_blob_rawsize;
    GitBranchCreate *git_branch_create;
    GitBranchDelete *git_branch_delete;
    GitBranchIteratorFree *git_branch_iterator_free;
    GitBranchIteratorNew *git_branch_iterator_new;
    GitBranchName *git_branch_name;
    GitBranchNext *git_branch_next;
    GitBranchSetUpstream *git_branch_set_upstream;
    GitBranchUpstream *git_branch_upstream;
    GitClone *git_clone;
    GitCommitAuthor *git_commit_author;
    GitCommitCommitter *git_commit_committer;
    GitCommitCreate *git_commit_create;
    GitCommitFree *git_commit_free;
    GitCommitId *git_commit_id;
    GitCommitLookup *git_commit_lookup;
    GitCommitMessage *git_commit_message;
    GitCommitParent *git_commit_parent;
    GitCommitParentCount *git_commit_parentcount;
    GitCommitTree *git_commit_tree;
    GitDiffFree *git_diff_free;
    GitDiffNumDeltas *git_diff_num_deltas;
    GitDiffTreeToTree *git_diff_tree_to_tree;
    GitErrorLast *git_error_last;
    GitLibgit2Init *git_libgit2_init;
    GitLibgit2Opts *git_libgit2_opts;
    GitLibgit2Shutdown *git_libgit2_shutdown;
    GitOidCmp *git_oid_cmp;
    GitOidFromStrN *git_oid_fromstrn;
    GitOidToStr *git_oid_tostr;
    GitReferenceFree *git_reference_free;
    GitReferenceLookup *git_reference_lookup;
    GitReferenceName *git_reference_name;
    GitReferenceNameToId *git_reference_name_to_id;
    GitRemoteCreate *git_remote_create;
    GitRemoteFetch *git_remote_fetch;
    GitRemoteFree *git_remote_free;
    GitRemoteLookup *git_remote_lookup;
    GitRemotePrune *git_remote_prune;
    GitRemotePush *git_remote_push;
    GitRepositoryFree *git_repository_free;
    GitRepositoryOpen *git_repository_open;
    GitReset *git_reset;
    GitRevwalkFree *git_revwalk_free;
    GitRevwalkHide *git_revwalk_hide;
    GitRevwalkNew *git_revwalk_new;
    GitRevwalkNext *git_revwalk_next;
    GitRevwalkPush *git_revwalk_push;
    GitRevwalkPushHead *git_revwalk_push_head;
    GitRevwalkSorting *git_revwalk_sorting;
    GitSignatureFree *git_signature_free;
    GitSignatureNew *git_signature_new;
    GitTreeEntryId *git_tree_entry_id;
    GitTreeEntryType *git_tree_entry_type;
    GitTreeFree *git_tree_free;
    GitTreeLookup *git_tree_lookup;
    GitTreeWalk *git_tree_walk;
    GitTreebuilderFree *git_treebuilder_free;
    GitTreebuilderNew *git_treebuilder_new;
    GitTreebuilderWrite *git_treebuilder_write;
} Git2Code;

static Git2Code git2 = {0};
static char global_temporary_clone_dir[MAX_PATH_LEN];