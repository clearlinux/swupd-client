#ifndef SWUPD_GLOBALS
#define SWUPD_GLOBALS

#define DEFAULT_MAX_RETRIES 3
#define DEFAULT_RETRY_DELAY 10

/**
 * @file
 * @brief Keep global variables for swupd flag and other values needed all over
 * swupd, as well as functions to handle them.
 */

#include <stdbool.h>

#include "lib/timelist.h"

#ifdef __cplusplus
extern "C" {
#endif

#define optarg_to_bool(_optarg) (_optarg ? str_to_bool(_optarg) : true)

enum user_interaction {
	INTERACTIVE = 0,
	NON_INTERACTIVE_ASSUME_YES,
	NON_INTERACTIVE_ASSUME_NO
};

/*
 * Global variables
 */
extern struct globals {
	bool allow_insecure_http;
	bool content_url_is_local;
	bool need_systemd_reexec;
	bool need_update_bootmanager;
	bool no_boot_update;
	bool no_scripts;
	bool sigcheck;
	bool sigcheck_latest;
	bool timecheck;
	bool wait_for_scripts;
	bool user_defined_cert_path;
	char **swupd_argv;
	char *cert_path;
	char *content_url;
	char *format_string;
	char *mounted_dirs;
	char *path_prefix;
	char *state_dir_cache;
	char *cache_dir;
	char *data_dir;
	char *version_url;
	int max_retries;
	int retry_delay;
	int skip_diskspace_check;
	int skip_optional_bundles;
	int update_server_port;
	int user_interaction;
	timelist *global_times;
} globals;

/* Struct that holds the original values of some global variables,
 * useful to recover the values after modifying them */
extern struct globals_bkp {
	char *path_prefix;
	char *version_url;
	char *content_url;
	char *cache_dir;
	char *data_dir;
} globals_bkp;

struct global_options {
	const struct option *longopts;
	const unsigned int longopts_len;
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

unsigned int get_max_xfer(unsigned int default_max_xfer);
void save_cmd(char **argv);

bool set_path_prefix(char *path);
bool set_default_urls(void);
void set_default_path_prefix(void);
void set_content_url(char *url);
void set_version_url(char *url);
void set_cert_path(char *path);

#ifdef __cplusplus
}
#endif
#endif
