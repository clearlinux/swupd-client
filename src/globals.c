/*
 *   Software Updater - client side
 *
 *      Copyright © 2014-2016 Intel Corporation.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 2 or later of the License.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Authors:
 *         Regis Merlino <regis.merlino@intel.com>
 *         Tim Pepper <timothy.c.pepper@linux.intel.com>
 *
 */

#define _GNU_SOURCE
#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "lib/log.h"
#include "swupd.h"

#define NO_PROGRESS 1000
#define WAIT_FOR_SCRIPTS 1001

int allow_insecure_http = 0;
bool allow_mix_collisions = false;
bool migrate = false;
bool sigcheck = true;
bool timecheck = true;
bool no_scripts = false;
bool no_boot_update = false;
bool verify_esp_only;
bool verify_bundles_only = false;
int update_count = 0;
int update_skip = 0;
bool need_update_boot = false;
bool need_update_bootloader = false;
bool need_systemd_reexec = false;
bool update_complete = false;
bool ignore_orphans = true;
char *format_string = NULL;
char *path_prefix = NULL; /* must always end in '/' */
char *mounted_dirs = NULL;
char *bundle_to_add = NULL;
char *state_dir = NULL;
int skip_diskspace_check = 0;
int skip_optional_bundles = 0;
bool keepcache = false;
timelist *global_times = NULL;
int max_retries = 3;
int retry_delay = 10;
bool wait_for_scripts = false;

/* NOTE: Today the content and version server urls are the same in
 * all cases.  It is highly likely these will eventually differ, eg:
 * swupd-version.01.org and swupd-files.01.org as this enables
 * different quality of server and control of the servers
 */
bool verbose_time = false;
bool have_manifest_diskspace = false; /* assume no until checked */
char *version_url = NULL;
char *content_url = NULL;
bool content_url_is_local = false;
char *cert_path = NULL;
int update_server_port = -1;
static int max_parallel_downloads = -1;
static int log_level = LOG_INFO;
char **swupd_argv = NULL;

/* flag required to identify options being set by a config file
 * and options being set by a flag in a command, the latter should
 * have higher precedence */
static bool from_config = false;

void global_set_opt_from_config(bool state)
{
	from_config = state;
}

/* If the MIX_BUNDLES_DIR has the valid-mix flag file we can run through
 * adding the mix data to the OS */
bool check_mix_exists(void)
{
	char *fullpath;
	bool ret;
	string_or_die(&fullpath, "%s%s/.valid-mix", path_prefix, MIX_DIR);
	ret = access(fullpath, F_OK) == 0;
	free_string(&fullpath);
	return ret;
}

/* Once system is on mix this file should exist */
bool system_on_mix(void)
{
	bool ret = (access(MIXED_FILE, R_OK) == 0);
	return ret;
}

int get_value_from_path(char **contents, const char *path, bool is_abs_path)
{
	char line[LINE_MAX];
	FILE *file;
	char *c;
	char *rel_path;
	int ret = -1;

	/* When path is an absolute path, do not prepend the path_prefix. This
	 * happens when this function is called with state_dir as the path, which
	 * already has the path_prefix included */
	if (is_abs_path) {
		string_or_die(&rel_path, path);
	} else {
		string_or_die(&rel_path, "%s%s", path_prefix, path);
	}

	file = fopen(rel_path, "r");
	if (!file) {
		free_string(&rel_path);
		return ret;
	}

	/* the file should contain exactly one line */
	line[0] = 0;
	if (fgets(line, LINE_MAX, file) == NULL) {
		if (ferror(file)) {
			error("Unable to read data from %s\n", rel_path);
		} else if (feof(file)) {
			error("Contents of %s are empty\n", rel_path);
		}
		goto fail;
	}

	/* remove newline if present */
	c = strchr(line, '\n');
	if (c) {
		*c = '\0';
	}

	string_or_die(contents, line);
	ret = 0;
fail:
	fclose(file);
	free_string(&rel_path);
	return ret;
}

int get_version_from_path(const char *abs_path)
{
	int ret = -1;
	int val;
	char *ret_str;

	ret = get_value_from_path(&ret_str, abs_path, true);
	if (ret == 0) {
		int err = strtoi_err(ret_str, &val);
		free_string(&ret_str);

		if (err != 0) {
			error("Invalid version\n");
		} else {
			return val;
		}
	}

	return -1;
}

/* Initializes the content_url global variable with the default value,
 * the value is read from the 'contenturl' configuration file */
int set_default_content_url(void)
{
	int ret;

	// Look for mirror inside path_prefix
	ret = get_value_from_path(&content_url, MIRROR_CONTENT_URL_PATH, false);
	if (ret == 0) {
		goto end;
	}

	// Look for config file in /usr/share inside path_prefix
	ret = get_value_from_path(&content_url, DEFAULT_CONTENT_URL_PATH, false);
	if (ret == 0) {
		goto end;
	}

	// Look for config file in /usr/share
	ret = get_value_from_path(&content_url, DEFAULT_CONTENT_URL_PATH, true);
	if (ret == 0) {
		goto end;
	}

	return -1;
end:
	remove_trailing_slash(content_url);
	content_url_is_local = strncmp(content_url, "file://", 7) == 0;
	return 0;
}

/* Sets the content_url global variable */
void set_content_url(char *url)
{
	static bool set_by_config = false;

	/* normally we want to only set this once; we assume the first successful
	 * set is the best choice, except when the first set was done by a config
	 * file, that can be overwritten by other values set from command line */
	if (content_url && !set_by_config) {
		return;
	}

	free_string(&content_url);
	content_url = strdup_or_die(url);
	remove_trailing_slash(content_url);
	content_url_is_local = strncmp(content_url, "file://", 7) == 0;

	/* if this option is being set by an option in a configuration file,
	 * set the local flag */
	set_by_config = from_config ? true : false;
}

/* Initializes the version_url global variable with the default value,
 * the value is read from the 'versionurl' configuration file */
int set_default_version_url(void)
{
	int ret;

	// Look for mirror inside path_prefix
	ret = get_value_from_path(&version_url, MIRROR_VERSION_URL_PATH, false);
	if (ret == 0) {
		goto end;
	}

	// Look for config file in /usr/share inside path_prefix
	ret = get_value_from_path(&version_url, DEFAULT_VERSION_URL_PATH, false);
	if (ret == 0) {
		goto end;
	}

	// Look for config file in /usr/share
	ret = get_value_from_path(&version_url, DEFAULT_VERSION_URL_PATH, true);
	if (ret == 0) {
		goto end;
	}

	return -1;
end:
	remove_trailing_slash(version_url);
	return 0;
}

/* Sets the version_url global variable */
void set_version_url(char *url)
{
	static bool set_by_config = false;

	/* normally we want to only set this once; we assume the first successful
	 * set is the best choice, except when the first set was done by a config
	 * file, that can be overwritten by other values set from command line */
	if (version_url && !set_by_config) {
		return;
	}

	free_string(&version_url);
	version_url = strdup_or_die(url);
	remove_trailing_slash(version_url);

	/* if this option is being set by an option in a configuration file,
	 * set the local flag */
	set_by_config = from_config ? true : false;
}

static bool is_valid_integer_format(char *str)
{
	unsigned long long int version;
	char *endptr = NULL;
	errno = 0;

	version = strtoull(str, &endptr, 10);
	if ((errno != 0) || (version == 0) || (endptr && *endptr != '\0')) {
		return false;
	}

	return true;
}

/* Initializes the state_dir global variable. If the path parameter is not
 * NULL, state_dir will be set to its value. Otherwise, the value is the
 * build-time default (STATE_DIR).
 */
static bool set_state_dir(char *path)
{
	bool use_default = path ? false : true;

	if (!use_default) {
		if (path[0] != '/') {
			error("state dir must be a full path starting with '/', not '%c'\n", path[0]);
			return false;
		}

		/* Prevent some disasters: since the state dir can be destroyed and
		 * reconstructed, make sure we never set those by accident and nuke the
		 * system. */
		if (!strcmp(path, "/") || !strcmp(path, "/var") || !strcmp(path, "/usr")) {
			error("Refusing to use '%s' as a state dir because it might be erased first.\n", path);
			return false;
		}

		free_string(&state_dir);
		string_or_die(&state_dir, "%s", path);
	} else {
		/* initializing */
		if (state_dir) {
			return true;
		}
		string_or_die(&state_dir, "%s", STATE_DIR);
	}

	return true;
}

/* Initializes the format_string global variable. If the userinput parameter is
 * not NULL, format_string will be set to its value, but only if it is a
 * positive integer or the special value "staging". Otherwise, the value is
 * read from the 'format' configuration file.
 */
static bool set_format_string(char *userinput)
{
	int ret;
	bool use_default = userinput ? false : true;
	static bool set_by_config = false;

	if (format_string) {
		if (!set_by_config || use_default) {
			return true;
		} else {
			/* the value was originally set using a config file, but it is now being
			 * specified as a command flag */
			free_string(&format_string);
		}
	}

	if (!use_default) {
		// allow "staging" as a format string
		if ((strcmp(userinput, "staging") == 0)) {
			format_string = strdup_or_die(userinput);
			return true;
		}

		// otherwise, expect a positive integer
		if (!is_valid_integer_format(userinput)) {
			return false;
		}
		format_string = strdup_or_die(userinput);
	} else {
		/* no option passed; use the default value */
		ret = get_value_from_path(&format_string, DEFAULT_FORMAT_PATH, false);

		// Fallback to system format ID
		if (ret < 0) {
			ret = get_value_from_path(&format_string, DEFAULT_FORMAT_PATH, true);
		}

		if (ret < 0) {
			return false;
		}

		if (!is_valid_integer_format(format_string)) {
			free_string(&format_string);
			return false;
		}
	}

	/* if this option is being set by an option in a configuration file,
	 * set the local flag */
	set_by_config = from_config ? true : false;

	return true;
}

/* Initializes the path_prefix global variable. If the path parameter is not
 * NULL, path_prefix will be set to its value.
 * Otherwise, the default value of '/'
 * is used. Note that the given path must exist.
 */
bool set_path_prefix(char *path)
{
	struct stat statbuf;
	int ret;
	bool use_default = path ? false : true;

	if (!use_default) {
		int len;
		char *tmp;
		char real_path[PATH_MAX] = { 0 };

		/* in case multiple -p options are passed */
		free_string(&path_prefix);
		string_or_die(&tmp, "%s", path);

		/* ensure path_prefix is fully resolved and at least ends in '/',
		 * and is a valid dir */
		if (tmp[0] != '/') {
			char *cwd;

			cwd = get_current_dir_name();
			if (cwd == NULL) {
				error("Unable to get current directory name (%s)\n", strerror(errno));
				free_string(&tmp);
				return false;
			}

			free_string(&tmp);
			string_or_die(&tmp, "%s/%s", cwd, path);
			free_string(&cwd);
		}

		if (!realpath(tmp, real_path)) {
			error("Bad path_prefix %s (%s), cannot continue.\n",
			      path_prefix, strerror(errno));
			free_string(&tmp);
			return false;
		}
		free_string(&tmp);
		string_or_die(&tmp, "%s/", real_path);

		len = strlen(tmp);
		if (!len || (tmp[len - 1] != '/')) {
			char *tmp_old = tmp;
			string_or_die(&tmp, "%s/", tmp_old);
			free_string(&tmp_old);
		}

		path_prefix = tmp;

	} else {
		/* initializing */
		if (path_prefix) {
			/* option passed on command line previously */
			return true;
		} else {
			string_or_die(&path_prefix, "/");
		}
	}
	ret = stat(path_prefix, &statbuf);
	if (ret != 0 || !S_ISDIR(statbuf.st_mode)) {
		error("Bad path_prefix %s (%s), cannot continue.\n",
		      path_prefix, strerror(errno));
		return false;
	}

	return true;
}

/* Initializes the cert_path global variable. If the path parameter is not
 * NULL, cert_path will be set to its value. Otherwise, the default build-time
 * value is used (CERT_PATH). Note that only the first call to this function
 * sets the variable.
 */
static void set_cert_path(char *path)
{
	bool use_default = path ? false : true;
	static bool set_by_config = false;

	// Early exit if the function was called previously.
	if (cert_path) {
		if (!set_by_config || use_default) {
			return;
		} else {
			/* the value was originally set using a config file, but it is now being
			 * specified as a command flag */
			free_string(&cert_path);
		}
	}

	/* Cmd line has priority, otherwise check if we're on a mix so the correct
	 * cert is used and user does not have to call swupd with -C all the time */
	if (!use_default) {
		string_or_die(&cert_path, "%s", path);
	} else {
		if (system_on_mix()) {
			string_or_die(&cert_path, "%s", MIX_CERT);
		} else {
			// CERT_PATH is guaranteed to be valid at this point.
			string_or_die(&cert_path, "%s", CERT_PATH);
		}
	}

	/* if this option is being set by an option in a configuration file,
	 * set the local flag */
	set_by_config = from_config ? true : false;
}

bool init_globals(void)
{
	int ret;

	ret = set_state_dir(NULL);
	if (!ret) {
		return false;
	}

	ret = set_path_prefix(NULL);
	/* a valid path prefix must be set to continue */
	if (!ret) {
		return false;
	}

	/* Set defaults with following order of preference:
	1. Runtime flags
	2. State dir configuration files
	3. Configure time settings

	Calling with NULL means use the default config file value
*/
	if (!set_format_string(NULL)) {
#ifdef FORMATID
		/* Fallback to configure time format_string if other sources fail */
		set_format_string(FORMATID);
#else
		error("Unable to determine format id. Use the -F option instead.\n");
		exit(EXIT_FAILURE);
#endif
	}

	if (!version_url) {
		if (set_default_version_url()) {
#ifdef VERSIONURL
			/* Fallback to configure time version_url if other sources fail */
			set_version_url(VERSIONURL);
#endif
			if (!version_url) {
				error("\nDefault version URL not found. Use the -v option instead.\n");
				exit(EXIT_FAILURE);
			}
		}
	}

	if (!content_url) {
		if (set_default_content_url()) {
#ifdef CONTENTURL
			/* Fallback to configure time content_url if other sources fail */
			set_content_url(CONTENTURL);
#endif
			if (!content_url) {
				error("\nDefault content URL not found. Use the -c option instead.\n");
				exit(EXIT_FAILURE);
			}
		}
	}

	set_cert_path(NULL);

	if (verbose_time) {
		global_times = timelist_new();
	}

	return true;
}

void globals_deinit(void)
{
	/* freeing all globals and set ALL them to NULL (via free_string)
	 * to avoid memory corruption on multiple calls
	 * to swupd_init() */
	free_string(&content_url);
	free_string(&version_url);
	free_string(&path_prefix);
	free_string(&format_string);
	free_string(&mounted_dirs);
	free_string(&state_dir);
	free_string(&bundle_to_add);
	timelist_free(global_times);
	global_times = NULL;
}

void save_cmd(char **argv)
{
	swupd_argv = argv;
}

size_t get_max_xfer(size_t default_max_xfer)
{
	if (max_parallel_downloads > 0) {
		return max_parallel_downloads;
	}

	return default_max_xfer;
}

static const struct option global_opts[] = {
	{ "certpath", required_argument, 0, 'C' },
	{ "contenturl", required_argument, 0, 'c' },
	{ "format", required_argument, 0, 'F' },
	{ "help", no_argument, 0, 'h' },
	{ "ignore-time", no_argument, 0, 'I' },
	{ "max-parallel-downloads", required_argument, 0, 'W' },
	{ "no-boot-update", no_argument, 0, 'b' },
	{ "no-scripts", no_argument, 0, 'N' },
	{ "no-progress", no_argument, 0, NO_PROGRESS },
	{ "nosigcheck", no_argument, 0, 'n' },
	{ "path", required_argument, 0, 'p' },
	{ "port", required_argument, 0, 'P' },
	{ "statedir", required_argument, 0, 'S' },
	{ "time", no_argument, 0, 't' },
	{ "url", required_argument, 0, 'u' },
	{ "versionurl", required_argument, 0, 'v' },
	{ "quiet", no_argument, &log_level, LOG_ERROR },
	{ "debug", no_argument, &log_level, LOG_DEBUG },
	{ "max-retries", required_argument, 0, 'r' },
	{ "retry-delay", required_argument, 0, 'd' },
	{ "json-output", no_argument, 0, 'j' },
	{ "allow-insecure-http", no_argument, &allow_insecure_http, 1 },
	{ "wait-for-scripts", no_argument, 0, WAIT_FOR_SCRIPTS },
	{ 0, 0, 0, 0 }
};

static bool global_parse_opt(int opt, char *optarg)
{
	int err;

	switch (opt) {
	case 0:
		/* Handle options that don't have shortcut. */
		return true;
	case 'p': /* default empty path_prefix verifies the running OS */
		if (!set_path_prefix(optarg)) {
			error("Invalid --path argument\n\n");
			return false;
		}
		return true;
	case 'u':
		set_version_url(optarg);
		set_content_url(optarg);
		return true;
	case 'P':
		err = strtoi_err(optarg, &update_server_port);
		if (err < 0 || update_server_port < 0) {
			error("Invalid --port argument: %s\n\n", optarg);
			return false;
		}
		return true;
	case 'c':
		set_content_url(optarg);
		return true;
	case 'v':
		set_version_url(optarg);
		return true;
	case 'F':
		if (!set_format_string(optarg)) {
			error("Invalid --format argument\n\n");
			return false;
		}
		return true;
	case 'S':
		if (!set_state_dir(optarg)) {
			error("Invalid --statedir argument\n\n");
			return false;
		}
		return true;
	case 'n':
		if (optarg != NULL) {
			sigcheck = strtobool(optarg);
		} else {
			sigcheck = false;
		}
		return true;
	case 'I':
		if (optarg != NULL) {
			timecheck = strtobool(optarg);
		} else {
			timecheck = false;
		}
		return true;
	case 't':
		if (optarg != NULL) {
			verbose_time = strtobool(optarg);
		} else {
			verbose_time = true;
		}
		return true;
	case 'N':
		if (optarg != NULL) {
			no_scripts = strtobool(optarg);
		} else {
			no_scripts = true;
		}
		return true;
	case 'b':
		if (optarg != NULL) {
			no_boot_update = strtobool(optarg);
		} else {
			no_boot_update = true;
		}
		return true;
	case 'C':
		set_cert_path(optarg);
		return true;
	case 'W':
		err = strtoi_err(optarg, &max_parallel_downloads);
		if (err < 0 || max_parallel_downloads <= 0) {
			error("Invalid --max-parallel-downloads argument: %s\n\n", optarg);
			return false;
		}
		return true;
	case 'r':
		err = strtoi_err(optarg, &max_retries);
		if (err < 0 || max_retries < 0) {
			error("Invalid --max-retries argument: %s\n\n", optarg);
			return false;
		}
		return true;
	case 'd':
		err = strtoi_err(optarg, &retry_delay);
		if (err < 0 || retry_delay < 0 || retry_delay > 60) {
			error("Invalid --retry-delay argument: %s (should be between 0 - %d seconds)\n\n", optarg, MAX_DELAY);
			return false;
		}
		return true;
	case 'j':
		if (optarg != NULL) {
			set_json_format(strtobool(optarg));
		} else {
			set_json_format(true);
		}
		return true;
	case NO_PROGRESS:
		if (optarg != NULL) {
			progress_disable(strtobool(optarg));
		} else {
			progress_disable(true);
		}
		return true;
	case WAIT_FOR_SCRIPTS:
		if (optarg != NULL) {
			wait_for_scripts = strtobool(optarg);
		} else {
			wait_for_scripts = true;
		}
		return true;
	default:
		return false;
	}

	return false;
}

static bool global_long_opt_default(const char *long_opt)
{

	if (strcmp(long_opt, "quiet") == 0) {
		log_level = LOG_INFO;
	} else if (strcmp(long_opt, "debug") == 0) {
		log_level = LOG_INFO;
	} else if (strcmp(long_opt, "allow-insecure-http") == 0) {
		allow_insecure_http = 0;
	} else {
		return false;
	}

	return true;
}

/* Generate the optstring required by getopt_long, based on options array */
static char *generate_optstring(struct option *opts, int num_opts)
{
	int i = 0;
	char *optstring = malloc(num_opts * 2 + 1); // Space for each opt + ':'
	ON_NULL_ABORT(optstring);

	while (opts->name) {
		if (isalpha(opts->val)) {
			optstring[i++] = opts->val;
			if (opts->has_arg) {
				optstring[i++] = ':';
			}
		}
		opts++;
	}
	optstring[i] = '\0';

	return optstring;
}

void global_print_help(void)
{
	print("Global Options:\n");
	print("   -h, --help              Show help options\n");
	print("   -p, --path=[PATH]       Use [PATH] as the path to verify (eg: a chroot or btrfs subvol)\n");
	print("   -u, --url=[URL]         RFC-3986 encoded url for version string and content file downloads\n");
	print("   -P, --port=[port #]     Port number to connect to at the url for version string and content file downloads\n");
	print("   -c, --contenturl=[URL]  RFC-3986 encoded url for content file downloads\n");
	print("   -v, --versionurl=[URL]  RFC-3986 encoded url for version file downloads\n");
	print("   -F, --format=[staging,1,2,etc.]  the format suffix for version file downloads\n");
	print("   -S, --statedir=[PATH]   Specify alternate swupd state directory\n");
	print("   -C, --certpath=[PATH]   Specify alternate path to swupd certificates\n");
	print("   -W, --max-parallel-downloads=[n] Set the maximum number of parallel downloads\n");
	print("   -r, --max-retries=[n]   Maximum number of retries for download failures\n");
	print("   -d, --retry-delay=[n]   Initial delay in seconds between download retries, this will be doubled for each retry\n");
	print("   -n, --nosigcheck        Do not attempt to enforce certificate or signature checking\n");
	print("   -I, --ignore-time       Ignore system/certificate time when validating signature\n");
	print("   -t, --time              Show verbose time output for swupd operations\n");
	print("   -N, --no-scripts        Do not run the post-update scripts and boot update tool\n");
	print("   -b, --no-boot-update    Do not install boot files to the boot partition (containers)\n");
	print("   -j, --json-output       Print all output as a JSON stream\n");
	print("   --allow-insecure-http   Allow updates over insecure connections\n");
	print("   --quiet                 Quiet output. Print only important information and errors\n");
	print("   --debug                 Print extra information to help debugging problems\n");
	print("   --no-progress           Don't print progress report\n");
	print("   --wait-for-scripts      Wait for the post-update scripts to complete\n");
	print("\n");
}

int global_parse_options(int argc, char **argv, const struct global_options *opts)
{
	struct option *opts_array;
	int num_global_opts, opt;
	char *optstring;
	int ret = 0;
	bool config_found = false;

	if (!opts) {
		return -EINVAL;
	}

	// If there's extra options
	num_global_opts = (sizeof(global_opts) / sizeof(struct option));
	opts_array = malloc(sizeof(struct option) * (opts->longopts_len + num_global_opts));
	ON_NULL_ABORT(opts_array);

	// Copy local and global options to opts
	memcpy(opts_array, opts->longopts,
	       opts->longopts_len * sizeof(struct option));
	memcpy(opts_array + opts->longopts_len, global_opts,
	       num_global_opts * sizeof(struct option));

	optstring = generate_optstring(opts_array,
				       num_global_opts + opts->longopts_len);

	// load configuration values from the config file first
	config_loader_init(argv[0], opts_array, global_parse_opt, opts->parse_opt, global_long_opt_default);
	if (CONFIG_FILE_PATH) {
		struct stat st;
		char *ctx = NULL;
		char *config_file = NULL;
		char *str = strdup_or_die(CONFIG_FILE_PATH);
		for (char *tok = strtok_r(str, ":", &ctx); tok; tok = strtok_r(NULL, ":", &ctx)) {
			if (stat(tok, &st)) {
				continue;
			}
			if ((st.st_mode & S_IFMT) != S_IFDIR) {
				continue;
			}
			debug("Looking for config file in path %s\n", tok);
			string_or_die(&config_file, "%s/config", tok);
			config_found = config_parse(config_file, config_loader_set_opt);
			free_string(&config_file);
			if (config_found) {
				debug("Using configuration file %s\n", tok);
				break;
			}
		}
		free_string(&str);
	}
	if (!config_found) {
		debug("Configuration file not found in %s\n", CONFIG_FILE_PATH);
		debug("No configuration file will be used\n\n");
	}

	// parse and load flags from command line
	while ((opt = getopt_long(argc, argv, optstring, opts_array, NULL)) != -1) {
		if (opt == 'h') {
			opts->print_help();
			exit(EXIT_SUCCESS);
			goto end;
		}

		// Try to parse local options if available
		if (opts && opts->parse_opt && opts->parse_opt(opt, optarg)) {
			continue;
		}

		// Try to parse global options
		if (global_parse_opt(opt, optarg)) {
			continue;
		}

		ret = -1;
		goto end;
	}

	log_set_level(log_level);

end:
	free(optstring);
	free(opts_array);
	return ret ? ret : optind;
}
