#ifndef SWUPD_GLOBALS
#define SWUPD_GLOBALS

#define DEFAULT_MAX_RETRIES 3
#define DEFAULT_RETRY_DELAY 10

/**
 * @file
 * @brief Keep global variables for swupd flag and other values needed all over
 * swupd, as well as functions to handle them.
 */

#include "lib/timelist.h"

#ifdef __cplusplus
extern "C" {
#endif

#define optarg_to_bool(_optarg) (_optarg ? strtobool(_optarg) : true)

/*
 * Global variables
 */
extern struct globals {
	bool allow_insecure_http;
	bool content_url_is_local;
	bool migrate;
	bool need_systemd_reexec;
	bool need_update_boot;
	bool need_update_bootloader;
	bool no_boot_update;
	bool no_scripts;
	bool sigcheck;
	bool timecheck;
	bool wait_for_scripts;
	char **swupd_argv;
	char *cert_path;
	char *content_url;
	char *format_string;
	char *mounted_dirs;
	char *path_prefix;
	char *state_dir;
	char *state_dir_cache;
	char *version_url;
	int max_retries;
	int retry_delay;
	int skip_diskspace_check;
	int skip_optional_bundles;
	int update_count;
	int update_server_port;
	int update_skip;
	timelist *global_times;
} globals;

/* Struct that holds the original values of some global variables,
 * useful to recover the values after modifying them */
extern struct globals_bkp {
	char *path_prefix;
	char *state_dir;
	char *version_url;
	char *content_url;
} globals_bkp;

struct global_options {
	const struct option *longopts;
	const int longopts_len;
	bool (*parse_opt)(int opt, char *optarg);
	void (*print_help)(void);
};

/**
 * @brief Parse command line options in argv calling the appropriate method in
 * opts.
 *
 * @param argc The number of elements in argv
 * @param argv A list of strings with the parameters to parse.
 * @param opts A struct global_options filled with information on how to handle
 * the flags in command line.
 */
int global_parse_options(int argc, char **argv, const struct global_options *opts);

bool globals_init(void);
void globals_deinit(void);
void global_print_help(void);

size_t get_max_xfer(size_t default_max_xfer);
void save_cmd(char **argv);

bool set_path_prefix(char *path);
bool set_default_urls(void);
bool set_state_dir_cache(char *path);
void set_default_path_prefix(void);
void set_content_url(char *url);
bool set_state_dir(char *path);
void set_version_url(char *url);
void set_cert_path(char *path);

#ifdef __cplusplus
}
#endif
#endif
