#ifndef __INCLUDE_GUARD_SWUPD_H
#define __INCLUDE_GUARD_SWUPD_H

#include <curl/curl.h>
#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/queue.h>

#include "list.h"
#include "swupd-error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* WARNING: keep SWUPD_VERSION_INCR in sync with server definition  */
#define SWUPD_VERSION_INCR 10
#define SWUPD_VERSION_IS_DEVEL(v) (((v) % SWUPD_VERSION_INCR) == 8)
#define SWUPD_VERSION_IS_RESVD(v) (((v) % SWUPD_VERSION_INCR) == 9)

#ifndef LINE_MAX
#define LINE_MAX _POSIX2_LINE_MAX
#endif

#define PATH_MAXLEN 4096
#define CURRENT_OS_VERSION -1

#define UNUSED_PARAM __attribute__((__unused__))

#define MAX_TRIES 3

#define SWUPD_HASH_DIRNAME "DIRECTORY"

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
	uint64_t contentsize;
	struct list *files;
	struct list *manifests;    /* struct file for possible manifests */
	struct list *submanifests; /* struct manifest for subscribed manifests */
	struct list *includes;     /* manifest names included in manifest */
	char *component;
};

struct version_container {
	size_t offset;
	char *version;
};

struct header;

extern bool verbose_time;
extern bool force;
extern bool sigcheck;
extern bool timecheck;
extern int verbose;
extern int update_count;
extern int update_skip;
extern bool update_complete;
extern bool need_update_boot;
extern bool need_update_bootloader;

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
	unsigned int is_tracked : 1;
	unsigned int is_manifest : 1;

	unsigned int is_config : 1;
	unsigned int is_state : 1;
	unsigned int is_boot : 1;
	unsigned int is_rename : 1;
	unsigned int is_orphan : 1;
	unsigned int do_not_update : 1;

	struct file *peer;      /* same file in another manifest */
	struct file *deltapeer; /* the file to do the binary delta against; often same as "peer" except in rename cases */
	struct header *header;

	char *staging; /* output name used during download & staging */
	CURL *curl;    /* curl handle if downloading */
	FILE *fh;      /* file written into during downloading */
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
extern bool local_download;
extern bool verify_esp_only;
extern bool have_manifest_diskspace;
extern bool have_network;
extern bool verify_bundles_only;
extern bool ignore_config;
extern bool ignore_state;
extern bool ignore_orphans;
extern char *format_string;
extern char *path_prefix;
extern bool set_format_string(char *userinput);
extern bool init_globals(void);
extern void free_globals(void);
extern char *bundle_to_add;
extern struct timeval start_time;
extern char *state_dir;

extern char *version_url;
extern char *content_url;
extern char *cert_path;
extern long update_server_port;
extern bool set_path_prefix(char *path);
extern int set_content_url(char *url);
extern int set_version_url(char *url);
extern void set_cert_path(char *path);
extern bool set_state_dir(char *path);

extern void check_root(void);
extern void clean_curl_multi_queue(void);
extern void increment_retries(int *retries, int *timeout);

extern int main_update(void);
extern int add_included_manifests(struct manifest *mom, int current, struct list **subs);
extern int main_verify(int current_version);

extern int get_latest_version(void);
extern void read_versions(int *current_version, int *server_version, char *path_prefix);
extern int check_versions(int *current_version, int *server_version, char *path_prefix);
extern int get_current_version(char *path_prefix);

extern bool check_network(void);

extern bool ignore(struct file *file);
extern void apply_heuristics(struct file *file);

extern int file_sort_filename(const void *a, const void *b);
extern int file_sort_filename_reverse(const void *a, const void *b);
extern struct manifest *load_mom(int version);
extern struct manifest *load_manifest(int current, int version, struct file *file, struct manifest *mom, bool header_only);
extern struct list *create_update_list(struct manifest *current, struct manifest *server);
extern void link_manifests(struct manifest *m1, struct manifest *m2);
extern void link_submanifests(struct manifest *m1, struct manifest *m2, struct list *subs1, struct list *subs2);
extern void free_manifest(struct manifest *manifest);
extern void remove_manifest_files(char *filename, int version, char *hash);

extern void grabtime_start(timelist *list, const char *name);
extern void grabtime_stop(timelist *list);
extern void print_time_stats(timelist *list);
extern timelist init_timelist(void);

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

extern int download_subscribed_packs(struct list *subs, bool required);

extern void try_delta(struct file *file);
extern void full_download(struct file *file);
extern int start_full_download(bool pipelining);
extern struct list *end_full_download(void);
extern int untar_full_download(void *data);

extern int do_staging(struct file *file, struct manifest *manifest);
extern int rename_all_files_to_final(struct list *updates);
extern int rename_staged_file_to_final(struct file *file);

extern int update_device_latest_version(int version);

extern int swupd_curl_init(void);
extern void swupd_curl_cleanup(void);
extern void swupd_curl_set_current_version(int v);
extern void swupd_curl_set_requested_version(int v);
extern double swupd_query_url_content_size(char *url);
extern CURLcode swupd_download_file_start(struct file *file);
extern CURLcode swupd_download_file_complete(CURLcode curl_ret, struct file *file);
extern int swupd_curl_get_file(const char *url, char *filename, struct file *file,
			       struct version_container *tmp_version, bool pack);
#define SWUPD_CURL_LOW_SPEED_LIMIT 1
#define SWUPD_CURL_CONNECT_TIMEOUT 30
#define SWUPD_CURL_RCV_TIMEOUT 120
extern CURLcode swupd_curl_set_basic_options(CURL *curl, const char *url);
void swupd_curl_test_resume(void);

extern void free_subscriptions(struct list **subs);
extern void read_subscriptions(void);
extern void read_subscriptions_alt(struct list **subs);
extern int component_subscribed(struct list *subs, char *component);
extern void set_subscription_versions(struct manifest *latest, struct manifest *current, struct list **subs);

extern void hash_assign(char *src, char *dest);
extern bool hash_equal(char *hash1, char *hash2);
extern bool hash_is_zeros(char *hash);
extern int compute_hash_lazy(struct file *file, char *filename);
extern int compute_hash(struct file *file, char *filename) __attribute__((warn_unused_result));

/* manifest.c */
extern struct list *recurse_manifest(struct manifest *manifest, struct list *subs, const char *component);
extern struct list *consolidate_files(struct list *files);
extern void debug_write_manifest(struct manifest *manifest, char *filename);
extern void populate_file_struct(struct file *file, char *filename);
extern bool verify_file(struct file *file, char *filename);
extern int verify_bundle_hash(struct manifest *manifest, struct file *bundle);
extern void unlink_all_staged_content(struct file *file);
extern void link_renames(struct list *newfiles, struct manifest *from_manifest);
extern void dump_file_descriptor_leaks(void);
extern int rm_staging_dir_contents(const char *rel_path);
extern void dump_file_info(struct file *file);
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

extern void run_scripts(void);
extern void run_preupdate_scripts(struct manifest *manifest);

/* lock.c */
int p_lockfile(void);
void v_lockfile(int fd);

/* helpers.c */
extern int swupd_rm(const char *path);
extern int rm_bundle_file(const char *bundle);
extern void print_manifest_files(struct manifest *m);
extern void swupd_deinit(int lock_fd, struct list **subs);
extern int swupd_init(int *lock_fd);
extern void copyright_header(const char *name);
extern void string_or_die(char **strp, const char *fmt, ...);
void update_motd(int new_release);
void delete_motd(void);
extern int get_dirfd_path(const char *fullname);
extern int verify_fix_path(char *targetpath, struct manifest *manifest);
extern void set_local_download(void);
extern struct list *files_from_bundles(struct list *bundles);

/* subscription.c */
struct list *free_list_file(struct list *item);
struct list *free_bundle(struct list *item);
extern void create_and_append_subscription(struct list **subs, const char *component);

/* bundle.c */
extern bool is_tracked_bundle(const char *bundle_name);
extern int remove_bundle(const char *bundle_name);
extern int list_installable_bundles();
extern int install_bundles_frontend(char **bundles);
extern int add_subscriptions(struct list *bundles, struct list **subs, int current_version, struct manifest *mom, int recursion);
void read_local_bundles(struct list **list_bundles);

/* telemetry.c */
typedef enum telem_prio_t {
	TELEMETRY_DEBG = 1,
	TELEMETRY_INFO,
	TELEMETRY_WARN,
	TELEMETRY_CRIT
} telem_prio_t;
extern void telemetry(telem_prio_t level, const char *class, const char *fmt, ...);

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
