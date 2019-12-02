#ifndef __INCLUDE_GUARD_SWUPD_H
#define __INCLUDE_GUARD_SWUPD_H

#include <dirent.h>
#include <getopt.h>
#include <limits.h>
#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bundle.h"
#include "config.h"
#include "config_loader.h"
#include "globals.h"
#include "lib/archives.h"
#include "lib/config_file.h"
#include "lib/formatter_json.h"
#include "lib/list.h"
#include "lib/log.h"
#include "lib/macros.h"
#include "lib/strings.h"
#include "lib/sys.h"
#include "lib/timelist.h"
#include "manifest.h"
#include "progress.h"
#include "scripts.h"
#include "swupd_curl.h"
#include "swupd_exit_codes.h"
#include "swupd_progress.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LINE_MAX
#define LINE_MAX _POSIX2_LINE_MAX
#endif

#define PATH_MAXLEN 4096
#define CURRENT_OS_VERSION -1
#define BUNDLE_NAME_MAXLEN 256

/* value used for configuring downloads */
#define DELAY_MULTIPLIER 2
#define MAX_DELAY 60

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

enum swupd_init_config {
	SWUPD_ALL = 0,
	SWUPD_NO_ROOT = 0b001,
	SWUPD_NO_TIMECHECK = 0b100,

};

struct header;

extern uint64_t total_curl_sz;

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
	unsigned int is_experimental : 1;
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

struct file_counts {
	int checked;
	int missing;
	int replaced;
	int not_replaced;
	int mismatch;
	int fixed;
	int not_fixed;
	int extraneous;
	int deleted;
	int not_deleted;
	int picky_extraneous;
};

extern void check_root(void);

extern int add_included_manifests(struct manifest *mom, struct list **subs);
extern int main_verify(int current_version);
extern enum swupd_code walk_tree(struct manifest *, const char *, bool, const regex_t *, struct file_counts *);

extern int get_latest_version(char *v_url);
extern enum swupd_code read_versions(int *current_version, int *server_version, char *path_prefix);
extern int get_current_version(char *path_prefix);
extern bool get_distribution_string(char *path_prefix, char *dist);
extern int get_current_format(void);
extern int get_server_format(int server_version);

extern bool ignore(struct file *file);
extern void apply_heuristics(struct file *file);

extern int file_sort_filename(const void *a, const void *b);
extern int file_sort_filename_reverse(const void *a, const void *b);
extern int file_sort_hash(const void *a, const void *b);
extern struct manifest *load_mom(int version, bool mix_exists, int *err);
extern struct manifest *load_manifest(int version, struct file *file, struct manifest *mom, bool header_only, int *err);
extern struct manifest *load_manifest_full(int version, bool mix);
extern void link_manifests(struct manifest *m1, struct manifest *m2);
extern void link_submanifests(struct manifest *m1, struct manifest *m2, struct list *subs1, struct list *subs2, bool server);

extern int get_value_from_path(char **contents, const char *path, bool is_abs_path);
extern int version_get_absolute_latest(void);

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
}

static inline void account_new_bundle(void)
{
	swupd_stats[3]++;
}

static inline void account_deleted_bundle(void)
{
	swupd_stats[4]++;
}

static inline void account_changed_bundle(void)
{
	swupd_stats[5]++;
}

static inline void account_delta_hit(void)
{
	swupd_stats[6]++;
}

static inline void account_delta_miss(void)
{
	swupd_stats[7]++;
}

extern void print_statistics(int version1, int version2);

extern int download_fullfiles(struct list *files, int *num_downloads);
extern int download_subscribed_packs(struct list *subs, struct manifest *mom, bool required);
extern int download_zero_packs(struct list *bundles, struct manifest *mom);

extern void apply_deltas(struct manifest *current_manifest);
extern int untar_full_download(void *data);

extern enum swupd_code do_staging(struct file *file, struct manifest *manifest);
extern enum swupd_code staging_install_all_files(struct list *files, struct manifest *mom);
extern int rename_staged_file_to_final(struct file *file);

extern int update_device_latest_version(int version);

extern void free_subscriptions(struct list **subs);
extern void read_subscriptions(struct list **subs);
extern bool component_subscribed(struct list *subs, char *component);
extern void set_subscription_versions(struct manifest *latest, struct manifest *current, struct list **subs);

extern void hash_assign(const char *src, char *dest);
extern bool hash_equal(const char *hash1, const char *hash2);
extern bool hash_is_zeros(char *hash);
extern int compute_hash_lazy(struct file *file, char *filename);
extern enum swupd_code compute_hash(struct file *file, char *filename) __attribute__((warn_unused_result));

/* manifest.c */
/* Calculate the total contentsize of a manifest list */
extern long get_manifest_list_contentsize(struct list *manifests);
extern struct list *recurse_manifest(struct manifest *manifest, struct list *subs, const char *component, bool server, int *err);
extern struct list *consolidate_files(struct list *files);
extern struct list *filter_out_deleted_files(struct list *files);
extern struct list *filter_out_existing_files(struct list *to_install_files, struct list *installed_files);

extern void populate_file_struct(struct file *file, char *filename);
extern bool verify_file(struct file *file, char *filename);
extern bool verify_file_lazy(char *filename);
extern int verify_bundle_hash(struct manifest *manifest, struct file *bundle);
extern int rm_staging_dir_contents(const char *rel_path);
void free_file_data(void *data);
void remove_files_in_manifest_from_fs(struct manifest *m);
void deduplicate_files_from_manifest(struct manifest **m1, struct manifest *m2);
extern struct file *mom_search_bundle(struct manifest *mom, const char *bundlename);
extern struct file *search_file_in_manifest(struct manifest *manifest, const char *filename);

extern bool is_directory_mounted(const char *filename);
extern bool is_under_mounted_directory(const char *filename);

/* filedesc.c */
extern void dump_file_descriptor_leaks(void);
extern void record_fds(void);

/* lock.c */
int p_lockfile(void);
void v_lockfile(void);

/* helpers.c */
extern void print_manifest_files(struct manifest *m);
extern void swupd_deinit(void);
enum swupd_code swupd_init(enum swupd_init_config config);
void update_motd(int new_release);
void delete_motd(void);
extern int get_dirfd_path(const char *fullname);
extern enum swupd_code verify_fix_path(char *targetpath, struct manifest *manifest);
extern struct list *consolidate_files_from_bundles(struct list *bundles);
extern struct list *files_from_bundles(struct list *bundles);
extern bool version_files_consistent(void);
extern bool string_in_list(char *string_to_check, struct list *list_to_check);
extern bool is_compatible_format(int format_num);
extern bool is_current_version(int version);
extern bool on_new_format(void);
extern char *get_printable_bundle_name(const char *bundle_name, bool is_experimental);
extern void print_regexp_error(int errcode, regex_t *regexp);
extern bool is_url_allowed(const char *url);
extern bool is_url_insecure(const char *url);
extern void remove_trailing_slash(char *url);
extern int link_or_copy(const char *orig, const char *dest);
extern int link_or_copy_all(const char *orig, const char *dest);
extern int remove_files_from_fs(struct list *files);
extern void print_pattern(const char *pattern, int times);
extern void prettify_size(long size_in_bytes, char **pretty_size);
extern int link_or_rename(const char *orig, const char *dest);
extern int create_state_dirs(const char *state_dir_path);

/* subscription.c */
struct list *free_list_file(struct list *item);
extern void create_and_append_subscription(struct list **subs, const char *component);
extern char *get_tracking_dir(void);
extern int add_subscriptions(struct list *bundles, struct list **subs, struct manifest *mom, bool find_all, int recursion);
extern int subscription_bundlename_strcmp(const void *a, const void *b);

/* bundle_add.c*/
extern enum swupd_code execute_bundle_add(struct list *bundles_list, int version);

/* verify.c */
extern enum swupd_code execute_verify(void);
extern void verify_set_option_download(bool opt);
extern void verify_set_option_skip_optional(bool opt);
extern void verify_set_option_force(bool opt);
extern void verify_set_option_install(bool opt);
extern void verify_set_option_statedir_cache(char *opt);
extern void verify_set_option_quick(bool opt);
extern void verify_set_option_bundles(struct list *opt_bundles);
extern void verify_set_option_version(int ver);
extern void verify_set_option_fix(bool opt);
extern void verify_set_option_picky(bool opt);
extern void verify_set_picky_whitelist(regex_t *whitelist);
extern void verify_set_picky_tree(char *picky_tree);
extern void verify_set_extra_files_only(bool opt);

/* repair.c */
extern regex_t *compile_whitelist(const char *whitelist_pattern);

/* bundle_list.c*/
extern enum swupd_code list_local_bundles(int version);
extern enum swupd_code list_installable_bundles(int version);
extern enum swupd_code show_included_bundles(char *bundle_name, int version);
extern enum swupd_code show_bundle_reqd_by(const char *bundle_name, bool server, int version);

/* telemetry.c */
typedef enum telem_prio_t {
	TELEMETRY_DEBG = 1,
	TELEMETRY_INFO,
	TELEMETRY_WARN,
	TELEMETRY_CRIT
} telem_prio_t;
extern void telemetry(telem_prio_t level, const char *class, const char *fmt, ...);

extern struct file **manifest_files_to_array(struct manifest *manifest);
extern int enforce_compliant_manifest(struct file **a, struct file **b, int searchsize, int size);
extern void manifest_free_array(struct file **array);

extern bool system_on_mix(void);
extern bool check_mix_exists(void);
extern void check_mix_versions(int *current_version, int *server_version, char *path_prefix);
extern int read_mix_version_file(char *filename, char *path_prefix);

extern enum swupd_code print_update_conf_info(void);

extern int handle_mirror_if_stale(void);

extern enum swupd_code clean_statedir(bool all, bool dry_run);

/* Parameter parsing in global.c */
extern struct global_const global;

enum swupd_code check_update();

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
