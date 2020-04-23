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
#include "config_loader.h"
#include "globals.h"
#include "lib/archives.h"
#include "lib/comp_functions.h"
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
#include "swupd_comp_functions.h"
#include "swupd_curl.h"
#include "swupd_exit_codes.h"
#include "swupd_progress.h"
#include "telemetry.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LINE_MAX
#define LINE_MAX _POSIX2_LINE_MAX
#endif

#define CURRENT_OS_VERSION -1
#define BUNDLE_NAME_MAXLEN 256

/* value used for configuring downloads */
#define DELAY_MULTIPLIER 2
#define MAX_DELAY 60

#define SWUPD_HASH_DIRNAME "DIRECTORY"
#define SWUPD_DEFAULTS "/usr/share/defaults/swupd/"
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

	/* is_optional: true if the subscription referred to by this struct is optional,
	 * false otherwise */
	bool is_optional;
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
	unsigned int is_experimental : 1;
	unsigned int is_exported : 1;
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
extern int get_int_from_url(const char *url);
extern enum swupd_code read_versions(int *current_version, int *server_version, char *path_prefix);
extern int get_current_version(char *path_prefix);
extern bool get_distribution_string(char *path_prefix, char *dist);
extern int get_current_format(void);
extern int get_server_format(int server_version);

extern bool ignore(struct file *file);
extern void apply_heuristics(struct file *file);

extern struct manifest *load_mom(int version, int *err);
extern struct manifest *load_manifest(int version, struct file *file, struct manifest *mom, bool header_only, int *err);
extern void link_manifests(struct manifest *m1, struct manifest *m2);
extern void link_submanifests(struct manifest *m1, struct manifest *m2, struct list *subs1, struct list *subs2, bool server);

extern int get_value_from_path(char **contents, const char *path, bool is_abs_path);
extern int version_get_absolute_latest(void);

static inline int bsearch_file_helper(const void *A, const void *B)
{
	struct file *key = (struct file *)A;
	struct file *elem = (*(struct file **)B);
	return str_cmp(key->filename, elem->filename);
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
extern struct list *consolidate_files_from_bundles(struct list *bundles);
extern struct list *files_from_bundles(struct list *bundles);
extern bool version_files_consistent(void);
extern bool string_in_list(char *string_to_check, struct list *list_to_check);
extern bool is_compatible_format(int format_num);
extern bool is_current_version(int version);
extern bool on_new_format(void);
extern char *get_printable_bundle_name(const char *bundle_name, bool is_experimental, bool is_installed, bool is_tracked);
extern void print_regexp_error(int errcode, regex_t *regexp);
extern bool is_url_allowed(const char *url);
extern bool is_url_insecure(const char *url);
extern void remove_trailing_slash(char *url);
extern void print_header(const char *header);
extern void prettify_size(long size_in_bytes, char **pretty_size);
extern int create_state_dirs(const char *state_dir_path);
extern bool confirm_action(void);
extern bool is_binary(const char *filename);

/* subscription.c */
typedef bool (*subs_fn_t)(struct list **subs, const char *component, int recursion, bool is_optional);
struct list *free_list_file(struct list *item);
extern void create_and_append_subscription(struct list **subs, const char *component);
extern char *get_tracking_dir(void);
extern int add_subscriptions(struct list *bundles, struct list **subs, struct manifest *mom, bool find_all, int recursion);
extern int subscription_get_tree(struct list *bundles, struct list **subs, struct manifest *mom, bool find_all, int recursion);

/* bundle_add.c */
typedef enum swupd_code (*extra_proc_fn_t)(struct list *files);
extern enum swupd_code bundle_add(struct list *bundles_list, int version);
extern enum swupd_code bundle_add_extra(struct list *bundles_list, int version, extra_proc_fn_t pre_add_fn, extra_proc_fn_t post_add_fn, extra_proc_fn_t file_validation_fn);
extern enum swupd_code execute_bundle_add(struct list *bundles_list);
extern enum swupd_code execute_bundle_add_extra(struct list *bundles_list, extra_proc_fn_t pre_add_fn, extra_proc_fn_t post_add_fn, extra_proc_fn_t file_validation_fn);

/* bundle_remove.c */
typedef enum swupd_code (*remove_extra_proc_fn_t)(struct list *removed_files, struct list *common_files);
extern enum swupd_code execute_remove_bundles(struct list *bundles);
extern enum swupd_code execute_remove_bundles_extra(struct list *bundles, remove_extra_proc_fn_t post_remove_fn);
extern void bundle_remove_set_option_force(bool opt);
extern void bundle_remove_set_option_recursive(bool opt);

/* bundle_info.c */
extern enum swupd_code bundle_info(char *bundle);
extern void bundle_info_set_option_version(int opt);
extern void bundle_info_set_option_dependencies(bool opt);
extern void bundle_info_set_option_files(bool opt);

/* update.c */
extern enum swupd_code execute_update(void);
extern enum swupd_code execute_update_extra(extra_proc_fn_t post_update_fn, extra_proc_fn_t file_validation_fn);
extern void update_set_option_version(int opt);
extern void update_set_option_download_only(bool opt);
extern void update_set_option_keepcache(bool opt);

/* verify.c */
extern enum swupd_code execute_verify(void);
extern enum swupd_code execute_verify_extra(extra_proc_fn_t post_verify_fn);
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
extern void verify_set_option_file(char *path);

/* repair.c */
extern regex_t *compile_whitelist(const char *whitelist_pattern);

/* bundle_list.c*/
extern enum swupd_code list_bundles(void);
extern void bundle_list_set_option_all(bool opt);
extern void bundle_list_set_option_has_dep(char *bundle);
extern void bundle_list_set_option_deps(char *bundle);
extern void bundle_list_set_option_status(bool opt);

/* clean.c */
extern int clean_get_stats(void);

extern struct file **manifest_files_to_array(struct manifest *manifest);
extern int enforce_compliant_manifest(struct file **a, struct file **b, int searchsize, int size);
extern void manifest_free_array(struct file **array);

extern enum swupd_code print_update_conf_info(void);

extern int handle_mirror_if_stale(void);

extern enum swupd_code clean_statedir(bool all, bool dry_run);

extern void warn_nosigcheck(const char *file);

extern void unlink_all_staged_content(struct file *file);

/* Parameter parsing in global.c */
extern struct global_const global;

/**
 * Free string and set it's value to NULL.
 */
extern void free_and_clear_pointer(char **s);

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
