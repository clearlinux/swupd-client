#ifndef __INCLUDE_GUARD_SWUPD_H
#define __INCLUDE_GUARD_SWUPD_H

#include <curl/curl.h>
#include <dirent.h>
#include <limits.h>
#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/queue.h>

#include "list.h"
#include "macros.h"
#include "swupd-error.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LINE_MAX
#define LINE_MAX _POSIX2_LINE_MAX
#endif

#define PATH_MAXLEN 4096
#define CURRENT_OS_VERSION -1
#define BUNDLE_NAME_MAXLEN 256

#define UNUSED_PARAM __attribute__((__unused__))

#define MAX_TRIES 3

#define SWUPD_HASH_DIRNAME "DIRECTORY"
#define SWUPD_DEFAULTS "/usr/share/defaults/swupd/"
#define MIX_DIR "/usr/share/mix/"
#define MIX_STATE_DIR MIX_DIR "update/www/"
#define MIX_CERT MIX_DIR "Swupd_Root.pem"
#define MIX_BUNDLES_DIR MIX_STATE_DIR "mix-bundles/"
#define MIXED_FILE SWUPD_DEFAULTS "mixed"
#define SSL_CLIENT_CERT "/etc/swupd/client.pem"

#define DEFAULT_VERSION_URL_PATH "/usr/share/defaults/swupd/versionurl"
#define MIRROR_VERSION_URL_PATH "/etc/swupd/mirror_versionurl"
#define DEFAULT_CONTENT_URL_PATH "/usr/share/defaults/swupd/contenturl"
#define MIRROR_CONTENT_URL_PATH "/etc/swupd/mirror_contenturl"
#define DEFAULT_FORMAT_PATH "/usr/share/defaults/swupd/format"

#define MIRROR_STALE_WARN 20
#define MIRROR_STALE_UNSET 500

#define DEFAULT_CONTENTURL "https://cdn.download.clearlinux.org/update/"

/* Return codes for adding subscriptions */
#define add_sub_ERR 1
#define add_sub_NEW 2
#define add_sub_BADNAME 4

/* download 404'd error code used by curl */
#define ENET404 404
struct sub {
	/* name of bundle/component/subscription */
	char *component;

	/* version meanings:
	 *   non-zero -> version has been read from MoM
	 *   -1       -> error, local bundle do not match any bundle on MoM */
	int version;

	/* oldversion: set to 0 by calloc(), possibly overridden by MoM read */
	int oldversion;
};

struct manifest {
	int version;
	int manifest_version;
	uint64_t filecount;
	uint64_t contentsize;
	struct list *files;
	struct list *manifests;    /* struct file for possible manifests */
	struct list *submanifests; /* struct manifest for subscribed manifests */
	struct list *includes;     /* manifest names included in manifest */
	char *component;
	unsigned int is_mix : 1;
};

struct curl_file_data {
	size_t capacity;
	size_t len;
	char *data;
};

struct header;

extern bool allow_mix_collisions;
extern bool verbose_time;
extern bool force;
extern bool migrate;
extern bool sigcheck;
extern bool timecheck;
extern int verbose;
extern int update_count;
extern int update_skip;
extern bool update_complete;
extern bool need_update_boot;
extern bool need_update_bootloader;
extern bool need_systemd_reexec;
extern struct list *post_update_actions;
extern bool keepcache;

struct update_stat {
	uint64_t st_mode;
	uint64_t st_uid;
	uint64_t st_gid;
	uint64_t st_rdev;
	uint64_t st_size;
};

#define DIGEST_LEN_SHA256 64
/* +1 for null termination */
#define SWUPD_HASH_LEN (DIGEST_LEN_SHA256 + 1)

struct file {
	char *filename;
	char hash[SWUPD_HASH_LEN];
	bool use_xattrs;
	int last_change;
	struct update_stat stat;

	unsigned int is_dir : 1;
	unsigned int is_file : 1;
	unsigned int is_link : 1;
	unsigned int is_deleted : 1;
	unsigned int is_ghosted : 1;
	unsigned int is_tracked : 1;
	unsigned int is_manifest : 1;

	unsigned int is_config : 1;
	unsigned int is_state : 1;
	unsigned int is_boot : 1;
	unsigned int is_orphan : 1;
	unsigned int is_mix : 1;
	unsigned int do_not_update : 1;

	struct file *peer; /* same file in another manifest */
	struct header *header;
	char *staging;
};

struct filerecord {
	char *filename;
	bool dir;
	bool in_manifest;
};

struct time {
	struct timespec procstart;
	struct timespec procstop;
	struct timespec rawstart;
	struct timespec rawstop;
	const char *name;
	bool complete;
	TAILQ_ENTRY(time)
	times;
};

typedef TAILQ_HEAD(timelist, time) timelist;

extern bool download_only;
extern bool verify_esp_only;
extern bool have_manifest_diskspace;
extern bool have_network;
extern bool verify_bundles_only;
extern bool ignore_config;
extern bool ignore_state;
extern bool ignore_orphans;
extern bool no_scripts;
extern bool no_boot_update;
extern char *format_string;
extern char *path_prefix;
extern bool set_format_string(char *userinput);
extern bool init_globals(void);
extern void free_globals(void);
extern void save_cmd(char **argv);
extern char *swupd_cmd;
extern char *bundle_to_add;
extern struct timeval start_time;
extern char *state_dir;
extern int skip_diskspace_check;

extern char *version_url;
extern char *content_url;
extern bool content_url_is_local;
extern char *cert_path;
extern long update_server_port;
extern int max_parallel_downloads;
extern char *default_format_path;
extern bool set_path_prefix(char *path);
extern int set_content_url(char *url);
extern int set_version_url(char *url);
extern void set_cert_path(char *path);
extern bool set_state_dir(char *path);

extern void check_root(void);
extern void increment_retries(int *retries, int *timeout);

extern int add_included_manifests(struct manifest *mom, struct list **subs);
extern int main_verify(int current_version);
extern int walk_tree(struct manifest *, const char *, bool, const regex_t *);

extern int get_latest_version(char *v_url);
extern int read_versions(int *current_version, int *server_version, char *path_prefix);
extern int check_versions(int *current_version, int *server_version, int requested_version, char *path_prefix);
extern int get_current_version(char *path_prefix);

extern bool ignore(struct file *file);
extern void apply_heuristics(struct file *file);

extern int file_sort_filename(const void *a, const void *b);
extern int file_sort_filename_reverse(const void *a, const void *b);
extern struct manifest *load_mom_err(int version, bool latest, bool mix_exists, int *err);
extern struct manifest *load_mom(int version, bool latest, bool mix_exists);
extern struct manifest *load_manifest(int version, struct file *file, struct manifest *mom, bool header_only);
extern struct manifest *load_manifest_full(int version, bool mix);
extern struct list *create_update_list(struct manifest *server);
extern void link_manifests(struct manifest *m1, struct manifest *m2);
extern void link_submanifests(struct manifest *m1, struct manifest *m2, struct list *subs1, struct list *subs2, bool server);
extern void free_manifest(struct manifest *manifest);

extern void grabtime_start(timelist *list, const char *name);
extern void grabtime_stop(timelist *list);
extern void print_time_stats(timelist *list);
extern int get_value_from_path(char **contents, const char *path, bool is_abs_path);
extern int get_version_from_path(const char *abs_path);
extern timelist init_timelist(void);

extern int extract_to(const char *tarfile, const char *outputdir);

static inline int bsearch_file_helper(const void *A, const void *B)
{
	struct file *key = (struct file *)A;
	struct file *elem = (*(struct file **)B);
	return strcmp(key->filename, elem->filename);
}

extern int swupd_stats[];
static inline void account_new_file(void)
{
	swupd_stats[0]++;
}

static inline void account_deleted_file(void)
{
	swupd_stats[1]++;
}

static inline void account_changed_file(void)
{
	swupd_stats[2]++;
};

static inline void account_new_bundle(void)
{
	swupd_stats[3]++;
};

static inline void account_deleted_bundle(void)
{
	swupd_stats[4]++;
};

static inline void account_changed_bundle(void)
{
	swupd_stats[5]++;
};

static inline void account_delta_hit(void)
{
	swupd_stats[6]++;
};

static inline void account_delta_miss(void)
{
	swupd_stats[7]++;
};

extern void print_statistics(int version1, int version2);

extern int download_fullfiles(struct list *files, int *num_downloads);
extern int download_subscribed_packs(struct list *subs, struct manifest *mom, bool required);

extern void apply_deltas(struct manifest *current_manifest);
extern int untar_full_download(void *data);

extern int do_staging(struct file *file, struct manifest *manifest);
extern int rename_all_files_to_final(struct list *updates);
extern int rename_staged_file_to_final(struct file *file);

extern int update_device_latest_version(int version);

extern size_t get_max_xfer(size_t default_max_xfer);

/* curl.c */

extern int swupd_curl_init(void);
extern void swupd_curl_deinit(void);
extern double swupd_curl_query_content_size(char *url);
extern int swupd_curl_get_file(const char *url, char *filename);
extern int swupd_curl_get_file_memory(const char *url, struct curl_file_data *file_data);

/* download.c */
typedef bool (*swupd_curl_success_cb)(void *data);
typedef bool (*swupd_curl_error_cb)(int response, void *data);
typedef void (*swupd_curl_free_cb)(void *data);
extern void *swupd_curl_parallel_download_start(size_t max_xfer);
void swupd_curl_parallel_download_set_callbacks(void *handle, swupd_curl_success_cb success_cb, swupd_curl_error_cb error_cb, swupd_curl_free_cb free_cb);
extern int swupd_curl_parallel_download_enqueue(void *handle, const char *url, const char *filename, const char *hash, void *data);
extern int swupd_curl_parallel_download_end(void *handle, int *num_downloads);

extern void free_subscriptions(struct list **subs);
extern void read_subscriptions(struct list **subs);
extern int component_subscribed(struct list *subs, char *component);
extern void set_subscription_versions(struct manifest *latest, struct manifest *current, struct list **subs);

extern void hash_assign(const char *src, char *dest);
extern bool hash_equal(const char *hash1, const char *hash2);
extern bool hash_is_zeros(char *hash);
extern int compute_hash_lazy(struct file *file, char *filename);
extern int compute_hash(struct file *file, char *filename) __attribute__((warn_unused_result));

/* manifest.c */
extern struct list *recurse_manifest(struct manifest *manifest, struct list *subs, const char *component, bool server);
extern struct list *consolidate_files(struct list *files);
extern struct list *filter_out_existing_files(struct list *files);

extern void populate_file_struct(struct file *file, char *filename);
extern bool verify_file(struct file *file, char *filename);
extern bool verify_file_lazy(char *filename);
extern int verify_bundle_hash(struct manifest *manifest, struct file *bundle);
extern int rm_staging_dir_contents(const char *rel_path);
void free_file_data(void *data);
void free_manifest_data(void *data);
void remove_files_in_manifest_from_fs(struct manifest *m);
void deduplicate_files_from_manifest(struct manifest **m1, struct manifest *m2);
extern struct file *search_bundle_in_manifest(struct manifest *manifest, const char *bundlename);
extern struct file *search_file_in_manifest(struct manifest *manifest, const char *filename);

extern char *mounted_dirs;
extern char *mk_full_filename(const char *prefix, const char *path);
extern bool is_directory_mounted(const char *filename);
extern bool is_under_mounted_directory(const char *filename);
extern int is_populated_dir(char *dirname);
extern int copy_all(const char *src, const char *dst);
extern int mkdir_p(const char *dir);

extern void run_scripts(bool block);
extern void run_preupdate_scripts(struct manifest *manifest);

/* filedesc.c */
extern void dump_file_descriptor_leaks(void);
extern void record_fds(void);

/* lock.c */
int p_lockfile(void);
void v_lockfile(void);

/* helpers.c */
extern int swupd_rm(const char *path);
extern int rm_bundle_file(const char *bundle);
extern void print_manifest_files(struct manifest *m);
extern void swupd_deinit(void);
extern int swupd_init(void);
extern void string_or_die(char **strp, const char *fmt, ...);
char *strdup_or_die(const char *const str);
extern void free_string(char **s);
void update_motd(int new_release);
void delete_motd(void);
extern int get_dirfd_path(const char *fullname);
extern int verify_fix_path(char *targetpath, struct manifest *manifest);
extern struct list *files_from_bundles(struct list *bundles);
extern bool version_files_consistent(void);
extern bool string_in_list(char *string_to_check, struct list *list_to_check);
extern void print_progress(unsigned int count, unsigned int max);
extern bool is_compatible_format(int format_num);
extern bool is_current_version(int version);
extern bool on_new_format(void);

/* subscription.c */
struct list *free_list_file(struct list *item);
struct list *free_bundle(struct list *item);
extern void create_and_append_subscription(struct list **subs, const char *component);

/* bundle.c */
extern bool is_tracked_bundle(const char *bundle_name);
extern int remove_bundles(char **bundles);
extern int show_bundle_reqd_by(const char *bundle_name, bool server);
extern int show_included_bundles(char *bundle_name);
extern int list_installable_bundles();
extern int install_bundles_frontend(char **bundles);
extern int add_subscriptions(struct list *bundles, struct list **subs, struct manifest *mom, bool find_all, int recursion);
int list_local_bundles();
extern int link_or_rename(const char *orig, const char *dest);

/* telemetry.c */
typedef enum telem_prio_t {
	TELEMETRY_DEBG = 1,
	TELEMETRY_INFO,
	TELEMETRY_WARN,
	TELEMETRY_CRIT
} telem_prio_t;
extern void telemetry(telem_prio_t level, const char *class, const char *fmt, ...);

/* fs.c */
extern long get_available_space(const char *path);
/* Calculate the total contentsize of a manifest list */
extern long get_manifest_list_contentsize(struct list *manifests);

extern struct file **manifest_files_to_array(struct manifest *manifest);
extern int enforce_compliant_manifest(struct file **a, struct file **b, int searchsize, int size);
extern void free_manifest_array(struct file **array);

extern bool system_on_mix(void);
extern bool check_mix_exists(void);
extern void check_mix_versions(int *current_version, int *server_version, char *path_prefix);
extern int read_mix_version_file(char *filename, char *path_prefix);

extern void print_update_conf_info(void);

extern void handle_mirror_if_stale(void);

extern int clean_statedir(bool all, bool dry_run);

/* some disk sizes constants for the various features:
 *   ...consider adding build automation to catch at build time
 *      if the build's artifacts are larger than these thresholds */
#define MANIFEST_REQUIRED_SIZE (1024 * 1024 * 100) /* 100M */
#define FREE_MARGIN 10				   /* 10%  */

/****************************************************************/

#ifdef __cplusplus
}
#endif

#endif
