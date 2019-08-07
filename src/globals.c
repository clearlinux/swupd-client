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

/* These defines have to be different from the local options
 * so they don't interfere with each other */
#define FLAG_NO_PROGRESS 1000
#define FLAG_WAIT_FOR_SCRIPTS 1001
#define FLAG_QUIET 1002
#define FLAG_DEBUG 1003
#define FLAG_ALLOW_INSECURE_HTTP 1004

struct globals globals = {
	.sigcheck = true,
	.timecheck = true,
	.max_retries = DEFAULT_MAX_RETRIES,
	.retry_delay = DEFAULT_RETRY_DELAY,
	.update_server_port = -1,
};

/* NOTE: Today the content and version server urls are the same in
 * all cases.  It is highly likely these will eventually differ, eg:
 * swupd-version.01.org and swupd-files.01.org as this enables
 * different quality of server and control of the servers
 */
static bool verbose_time = false;
static int max_parallel_downloads = -1;
static int log_level = LOG_INFO;
static bool quiet = false;
static bool debug = false;

/* Sets the content_url global variable */
static void set_content_url(char *url)
{
	if (globals.content_url) {
		free_string(&globals.content_url);
	}

	globals.content_url = strdup_or_die(url);
	remove_trailing_slash(globals.content_url);
	globals.content_url_is_local = strncmp(globals.content_url, "file://", 7) == 0;
}

/* Initializes the content_url global variable with the default value,
 * the value is read from the 'contenturl' configuration file */
bool set_default_content_url(void)
{
	int ret;
	char *new_content_url = NULL;

	ret = get_value_from_path(&new_content_url, MIRROR_CONTENT_URL_PATH, false);
	if (ret == 0) {
		goto found;
	}

	// Look for config file in /usr/share inside path_prefix
	ret = get_value_from_path(&new_content_url, DEFAULT_CONTENT_URL_PATH, false);
	if (ret == 0) {
		goto found;
	}

	// Look for config file in /usr/share
	ret = get_value_from_path(&new_content_url, DEFAULT_CONTENT_URL_PATH, true);
	if (ret == 0) {
		goto found;
	}

#ifdef CONTENTURL
	/* Fallback to configure time content_url if other sources fail */
	set_content_url(CONTENTURL);
	return true;
#endif

	return false;

found:
	set_content_url(new_content_url);
	free_string(&new_content_url);
	return true;
}

/* Sets the version_url global variable */
static void set_version_url(char *url)
{
	if (globals.version_url) {
		free_string(&globals.version_url);
	}

	globals.version_url = strdup_or_die(url);
	remove_trailing_slash(globals.version_url);
}

/* Initializes the version_url global variable with the default value,
 * the value is read from the 'versionurl' configuration file */
bool set_default_version_url(void)
{
	int ret;
	char *new_version_url = NULL;

	// Look for mirror inside path_prefix
	ret = get_value_from_path(&new_version_url, MIRROR_VERSION_URL_PATH, false);
	if (ret == 0) {
		goto found;
	}

	// Look for config file in /usr/share inside path_prefix
	ret = get_value_from_path(&new_version_url, DEFAULT_VERSION_URL_PATH, false);
	if (ret == 0) {
		goto found;
	}

	// Look for config file in /usr/share
	ret = get_value_from_path(&new_version_url, DEFAULT_VERSION_URL_PATH, true);
	if (ret == 0) {
		goto found;
	}

#ifdef VERSIONURL
	/* Fallback to configure time version_url if other sources fail */
	set_version_url(VERSIONURL);
	return true;
#endif

	return false;

found:
	set_version_url(new_version_url);
	free_string(&new_version_url);
	return true;
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
	if (!path) {
		error("Statedir shouldn't be NULL\n");
		return false;
	}

	if (path[0] != '/') {
		error("State dir must be a full path starting with '/', not '%c'\n", path[0]);
		return false;
	}

	/* Prevent some disasters: since the state dir can be destroyed and
	 * reconstructed, make sure we never set those by accident and nuke the
	 * system. */
	if (!strcmp(path, "/") || !strcmp(path, "/var") || !strcmp(path, "/usr")) {
		error("Refusing to use '%s' as a state dir because it might be erased first\n", path);
		return false;
	}

	free_string(&globals.state_dir);
	string_or_die(&globals.state_dir, "%s", path);

	return true;
}

static void set_default_state_dir()
{
	string_or_die(&globals.state_dir, "%s", STATE_DIR);
}

static bool set_format_string(char *format)
{
	// allow "staging" as a format string
	// or any positive integer
	if (strcmp(format, "staging") != 0 &&
	    !is_valid_integer_format(format)) {
		error("Invalid format '%s'\n", format);
		return false;
	}

	if (globals.format_string) {
		free_string(&globals.format_string);
	}

	globals.format_string = strdup_or_die(format);
	return true;
}

static bool set_default_format_string()
{
	int ret;
	char *new_format_string;
	bool result = false;

	/* no option passed; use the default value */
	ret = get_value_from_path(&new_format_string, DEFAULT_FORMAT_PATH, false);
	if (ret == 0) {
		goto found;
	}

	// Fallback to system format ID
	ret = get_value_from_path(&new_format_string, DEFAULT_FORMAT_PATH, true);
	if (ret == 0) {
		goto found;
	}

#ifdef FORMATID
	/* Fallback to configure time format_string if other sources fail */
	return set_format_string(FORMATID);
#endif

	return false;

found:
	result = set_format_string(new_format_string);
	free_string(&new_format_string);
	return result;
}

/* Initializes the path_prefix global variable. If the path parameter is not
 * NULL, path_prefix will be set to its value.
 * Otherwise, the default value of '/'
 * is used. Note that the given path must exist.
 */
bool set_path_prefix(char *path)
{
	struct stat statbuf;
	int ret, len;
	char *new_path;

	if (!path) {
		error("Path prefix shouldn't be NULL\n");
		return false;
	}

	ret = stat(path, &statbuf);
	// If path doesn't exit tries to create it
	if (ret < 0 && errno == ENOENT) {
		if (mkdir_p(path) < 0) {
			goto error;
		}
		ret = stat(path, &statbuf);
	}
	if (ret < 0 || !S_ISDIR(statbuf.st_mode)) {
		goto error;
	}

	new_path = realpath(path, NULL);
	if (!new_path) {
		goto error;
	}

	len = strlen(new_path);
	if (!len || (new_path[len - 1] != '/')) {
		char *tmp;

		tmp = new_path;
		new_path = str_or_die("%s/", new_path);
		free(tmp);
	}

	free(globals.path_prefix);
	globals.path_prefix = new_path;
	return true;

error:
	error("Bad path_prefix %s (%s), cannot continue\n",
	      globals.path_prefix, strerror(errno));
	free(new_path);
	return false;
}

void set_default_path_prefix()
{
	free(globals.path_prefix);
	globals.path_prefix = strdup_or_die("/");
}

static void set_cert_path(char *path)
{
	if (globals.cert_path) {
		free_string(&globals.cert_path);
	}

	globals.cert_path = strdup_or_die(path);
}

static void set_default_cert_path()
{
	if (system_on_mix()) {
		set_cert_path(MIX_CERT);
	} else {
		// CERT_PATH is guaranteed to be valid at this point.
		set_cert_path(CERT_PATH);
	}
}

bool globals_init(void)
{
	if (!globals.state_dir) {
		set_default_state_dir();
	}

	if (!globals.path_prefix) {
		set_default_path_prefix();
	}

	if (!globals.format_string && !set_default_format_string()) {
		error("Unable to determine format id. Use the -F option instead\n");
		return false;
	}

	if (!globals.version_url && !set_default_version_url()) {
		error("Default version URL not found. Use the -v option instead\n");
		return false;
	}

	if (!globals.content_url && !set_default_content_url()) {
		error("Default content URL not found. Use the -c option instead\n");
		return false;
	}

	if (!globals.cert_path) {
		set_default_cert_path();
	}

	if (verbose_time) {
		globals.global_times = timelist_new();
	}

	return true;
}

void globals_deinit(void)
{
	/* freeing all globals and set ALL them to NULL (via free_string)
	 * to avoid memory corruption on multiple calls
	 * to swupd_init() */
	free_string(&globals.content_url);
	free_string(&globals.version_url);
	free_string(&globals.path_prefix);
	free_string(&globals.format_string);
	free_string(&globals.mounted_dirs);
	free_string(&globals.state_dir);
	timelist_free(globals.global_times);
	globals.global_times = NULL;
}

void save_cmd(char **argv)
{
	globals.swupd_argv = argv;
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
	{ "no-progress", no_argument, 0, FLAG_NO_PROGRESS },
	{ "nosigcheck", no_argument, 0, 'n' },
	{ "path", required_argument, 0, 'p' },
	{ "port", required_argument, 0, 'P' },
	{ "statedir", required_argument, 0, 'S' },
	{ "time", no_argument, 0, 't' },
	{ "url", required_argument, 0, 'u' },
	{ "versionurl", required_argument, 0, 'v' },
	{ "quiet", no_argument, 0, FLAG_QUIET },
	{ "debug", no_argument, 0, FLAG_DEBUG },
	{ "max-retries", required_argument, 0, 'r' },
	{ "retry-delay", required_argument, 0, 'd' },
	{ "json-output", no_argument, 0, 'j' },
	{ "allow-insecure-http", no_argument, 0, FLAG_ALLOW_INSECURE_HTTP },
	{ "wait-for-scripts", no_argument, 0, FLAG_WAIT_FOR_SCRIPTS },
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
		err = strtoi_err(optarg, &globals.update_server_port);
		if (err < 0 || globals.update_server_port < 0) {
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
		globals.sigcheck = !optarg_to_bool(optarg);
		return true;
	case 'I':
		globals.timecheck = !optarg_to_bool(optarg);
		return true;
	case 't':
		verbose_time = optarg_to_bool(optarg);
		return true;
	case 'N':
		globals.no_scripts = optarg_to_bool(optarg);
		return true;
	case 'b':
		globals.no_boot_update = optarg_to_bool(optarg);
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
		err = strtoi_err(optarg, &globals.max_retries);
		if (err < 0 || globals.max_retries < 0) {
			error("Invalid --max-retries argument: %s\n\n", optarg);
			return false;
		}
		return true;
	case 'd':
		err = strtoi_err(optarg, &globals.retry_delay);
		if (err < 0 || globals.retry_delay < 0 || globals.retry_delay > 60) {
			error("Invalid --retry-delay argument: %s (should be between 0 - %d seconds)\n\n", optarg, MAX_DELAY);
			return false;
		}
		return true;
	case 'j':
		set_json_format(optarg_to_bool(optarg));
		return true;
	case FLAG_NO_PROGRESS:
		progress_disable(optarg_to_bool(optarg));
		return true;
	case FLAG_WAIT_FOR_SCRIPTS:
		globals.wait_for_scripts = optarg_to_bool(optarg);
		return true;
	case FLAG_QUIET:
		quiet = optarg_to_bool(optarg);
		return true;
	case FLAG_DEBUG:
		debug = optarg_to_bool(optarg);
		return true;
	case FLAG_ALLOW_INSECURE_HTTP:
		globals.allow_insecure_http = optarg_to_bool(optarg);
		return true;
	default:
		return false;
	}

	return false;
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
		if (opts->flag) {
			error("Flag should be NULL for optarg %s\n", opts->name);
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
	print("   -p, --path=[PATH]       Use [PATH] to set the top-level directory for the swupd-managed system\n");
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

static bool load_flags_in_config(char *command, struct option *opts_array, const struct global_options *opts)
{
	bool config_found = false;
#ifdef CONFIG_FILE_PATH
	// load configuration values from the config file
	struct config_loader_data loader_data = { command, opts_array, global_parse_opt, opts->parse_opt };
	struct stat st;
	char *ctx = NULL;
	char *config_file = NULL;
	char *str = strdup_or_die(CONFIG_FILE_PATH);

	for (char *tok = strtok_r(str, ":", &ctx); tok; tok = strtok_r(NULL, ":", &ctx)) {
		/* load values from all configuration files found, starting from
		 * the least important to the most one since values will be overwritten
		 * if found more than once */
		if (stat(tok, &st)) {
			continue;
		}
		if ((st.st_mode & S_IFMT) != S_IFDIR) {
			continue;
		}
		string_or_die(&config_file, "%s/config", tok);
		if (config_parse(config_file, config_loader_set_opt, &loader_data)) {
			config_found = true;
		}
		free_string(&config_file);
	}

	free_string(&str);
#endif
	return config_found;
}

int global_parse_options(int argc, char **argv, const struct global_options *opts)
{
	struct option *opts_array;
	int num_global_opts, opt;
	char *optstring;
	int ret = 0;
	bool config_found;

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

	config_found = load_flags_in_config(argv[0], opts_array, opts);

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

	if (debug) {
		log_level = LOG_DEBUG;
	} else if (quiet) {
		log_level = LOG_ERROR;
	}
	log_set_level(log_level);

	if (!config_found) {
		debug("No configuration file was found\n");
	}

end:
	free(optstring);
	free(opts_array);
	return ret ? ret : optind;
}
