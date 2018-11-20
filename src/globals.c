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
#include "swupd.h"
bool allow_mix_collisions = false;
bool force = false;
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
struct list *post_update_actions = NULL;
bool ignore_orphans = true;
char *format_string = NULL;
char *path_prefix = NULL; /* must always end in '/' */
char *mounted_dirs = NULL;
char *bundle_to_add = NULL;
char *state_dir = NULL;
int skip_diskspace_check = 0;
bool keepcache = false;
timelist *global_times = NULL;

/* NOTE: Today the content and version server urls are the same in
 * all cases.  It is highly likely these will eventually differ, eg:
 * swupd-version.01.org and swupd-files.01.org as this enables
 * different quality of server and control of the servers
 */
bool download_only;
bool verbose_time = false;
bool have_manifest_diskspace = false; /* assume no until checked */
char *version_url = NULL;
char *content_url = NULL;
bool content_url_is_local = false;
char *cert_path = NULL;
long update_server_port = -1;
static int max_parallel_downloads = -1;
char *swupd_cmd = NULL;

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
			fprintf(stderr, "Error: Unable to read data from %s\n", rel_path);
		} else if (feof(file)) {
			fprintf(stderr, "Error: Contents of %s are empty\n", rel_path);
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
			fprintf(stderr, "Error: Invalid version\n");
		} else {
			return val;
		}
	}

	return -1;
}

static int set_default_value_from_path(char **global, const char *path)
{
	int ret = -1;
	char *ret_str = NULL;

	ret = get_value_from_path(&ret_str, path, false);
	if (ret == 0 && ret_str) {
		string_or_die(global, "%s", ret_str);
		free_string(&ret_str);
	}

	return ret;
}

static int set_url(char **global, char *url, const char *path)
{
	int ret = 0;

	if (url) {
		free_string(global);
		string_or_die(global, "%s", url);
	} else {
		if (*global) {
			/* option passed on command line previously */
			return ret;
		} else {
			/* no option passed; use the default value */
			ret = set_default_value_from_path(global, path);
			return ret;
		}
	}

	return ret;
}

/* Initializes the content_url global variable. If the url parameter is not
 * NULL, content_url will be set to its value. Otherwise, the value is read
 * from the 'contenturl' configuration file.
 */
static int set_content_url(char *url)
{
	if (content_url) {
		/* Only set once; we assume the first successful set is the best choice */
		return 0;
	}

	/* check for mirror configuration first */
	if (set_url(&content_url, url, MIRROR_CONTENT_URL_PATH) == 0) {
		content_url_is_local = strncmp(content_url, "file://", 7) == 0;
		return 0;
	}

	int ret = set_url(&content_url, url, DEFAULT_CONTENT_URL_PATH);
	if (ret == 0) {
		content_url_is_local = strncmp(content_url, "file://", 7) == 0;
	}

	return ret;
}

/* Initializes the version_url global variable. If the url parameter is not
 * NULL, version_url will be set to its value. Otherwise, the value is read
 * from the 'versionurl' configuration file.
 */
static int set_version_url(char *url)
{
	if (version_url) {
		/* Only set once; we assume the first successful set is the best choice */
		return 0;
	}

	/* check for mirror configuration first */
	if (set_url(&version_url, url, MIRROR_VERSION_URL_PATH) == 0) {
		return 0;
	}

	return set_url(&version_url, url, DEFAULT_VERSION_URL_PATH);
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
	if (path) {
		if (path[0] != '/') {
			fprintf(stderr, "state dir must be a full path starting with '/', not '%c'\n", path[0]);
			return false;
		}

		/* Prevent some disasters: since the state dir can be destroyed and
		 * reconstructed, make sure we never set those by accident and nuke the
		 * system. */
		if (!strcmp(path, "/") || !strcmp(path, "/var") || !strcmp(path, "/usr")) {
			fprintf(stderr, "Refusing to use '%s' as a state dir because it might be erased first.\n", path);
			return false;
		}

		free_string(&state_dir);
		string_or_die(&state_dir, "%s", path);
	} else {
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

	if (format_string) {
		return true;
	}

	if (userinput) {
		// allow "staging" as a format string
		if ((strcmp(userinput, "staging") == 0)) {
			free_string(&format_string);
			string_or_die(&format_string, "%s", userinput);
			return true;
		}

		// otherwise, expect a positive integer
		if (!is_valid_integer_format(userinput)) {
			return false;
		}
		free_string(&format_string);
		string_or_die(&format_string, "%s", userinput);
	} else {
		/* no option passed; use the default value */
		ret = set_default_value_from_path(&format_string, DEFAULT_FORMAT_PATH);
		if (ret < 0) {
			return false;
		}
		if (!is_valid_integer_format(format_string)) {
			return false;
		}
	}

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

	if (path != NULL) {
		int len;
		char *tmp;

		/* in case multiple -p options are passed */
		free_string(&path_prefix);
		string_or_die(&tmp, "%s", path);

		/* ensure path_prefix is absolute, at least '/', ends in '/',
		 * and is a valid dir */
		if (tmp[0] != '/') {
			char *cwd;

			cwd = get_current_dir_name();
			if (cwd == NULL) {
				fprintf(stderr, "Unable to get current directory name (%s)\n", strerror(errno));
				free_string(&tmp);
				return false;
			}

			free_string(&tmp);
			string_or_die(&tmp, "%s/%s", cwd, path);
			free_string(&cwd);
		}

		len = strlen(tmp);
		if (!len || (tmp[len - 1] != '/')) {
			char *tmp_old = tmp;
			string_or_die(&tmp, "%s/", tmp_old);
			free_string(&tmp_old);
		}

		path_prefix = tmp;

	} else {
		if (path_prefix) {
			/* option passed on command line previously */
			return true;
		} else {
			string_or_die(&path_prefix, "/");
		}
	}
	ret = stat(path_prefix, &statbuf);
	if (ret != 0 || !S_ISDIR(statbuf.st_mode)) {
		fprintf(stderr, "Bad path_prefix %s (%s), cannot continue.\n",
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
#ifdef SIGNATURES
static void set_cert_path(char *path)
{
	// Early exit if the function was called previously.
	if (cert_path) {
		return;
	}

	/* Cmd line has priority, otherwise check if we're on a mix so the correct
	 * cert is used and user does not have to call swupd with -C all the time */
	if (path) {
		string_or_die(&cert_path, "%s", path);
	} else {
		if (system_on_mix()) {
			string_or_die(&cert_path, "%s", MIX_CERT);
		} else {
			// CERT_PATH is guaranteed to be valid at this point.
			string_or_die(&cert_path, "%s", CERT_PATH);
		}
	}
}
#else
static void set_cert_path(char UNUSED_PARAM *path)
{
	return;
}
#endif

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
		fprintf(stderr, "Unable to determine format id. Use the -F option instead.\n");
		exit(EXIT_FAILURE);
#endif
	}

	/* Calling with NULL means use the default config file value */
	if (set_version_url(NULL)) {
#ifdef VERSIONURL
		/* Fallback to configure time version_url if other sources fail */
		ret = set_version_url(VERSIONURL);
#else
		/* We have no choice but to fail */
		ret = -1;
#endif
		if (ret) {
			fprintf(stderr, "\nDefault version URL not found. Use the -v option instead.\n");
			exit(EXIT_FAILURE);
		}
	}

	/* Calling set_() with NULL means load the default config file value */
	if (set_content_url(NULL)) {
#ifdef CONTENTURL
		/* Fallback to configure time content_url if other sources fail */
		ret = set_content_url(CONTENTURL);
#else
		/* We have no choice but to fail */
		ret = -1;
#endif
		if (ret) {
			fprintf(stderr, "\nDefault content URL not found. Use the -c option instead.\n");
			exit(EXIT_FAILURE);
		}
	}

#ifdef SIGNATURES
	set_cert_path(NULL);
#endif

	if (verbose_time) {
		global_times = timelist_new();
	}

	return true;
}

void free_globals(void)
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
	list_free_list(post_update_actions);
	free_string(&bundle_to_add);
	timelist_free(global_times);
	global_times = NULL;
}

void save_cmd(char **argv)
{
	int size = 0;

	/* Find size of total argv strings */
	for (int i = 0; argv[i]; i++) {
		/* +1 for space (" ") after flag that will be added later */
		size += strlen(argv[i]) + 1;
	}

	/* +1 for null terminator */
	swupd_cmd = malloc(size + 1);
	ON_NULL_ABORT(swupd_cmd);

	strcpy(swupd_cmd, "");
	for (int i = 0; argv[i]; i++) {
		strcat(swupd_cmd, argv[i]);
		strcat(swupd_cmd, " ");
	}
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
	{ "nosigcheck", no_argument, 0, 'n' },
	{ "path", required_argument, 0, 'p' },
	{ "port", required_argument, 0, 'P' },
	{ "statedir", required_argument, 0, 'S' },
	{ "time", no_argument, 0, 't' },
	{ "url", required_argument, 0, 'u' },
	{ "versionurl", required_argument, 0, 'v' },
	//TODO: -D option is deprecated. Remove that on a Major release
	{ "", required_argument, 0, 'D' },
	{ 0, 0, 0, 0 }
};

static bool global_parse_opt(int opt, char *optarg)
{
	switch (opt) {
	case 0:
		/* Handle options that don't have shortcut. */
		return true;
	case 'p': /* default empty path_prefix verifies the running OS */
		if (!optarg || !set_path_prefix(optarg)) {
			fprintf(stderr, "Invalid --path argument\n\n");
			return false;
		}
		return true;
	case 'u':
		if (!optarg) {
			fprintf(stderr, "Invalid --url argument\n\n");
			return false;
		}
		set_version_url(optarg);
		set_content_url(optarg);
		return true;
	case 'P':
		if (sscanf(optarg, "%ld", &update_server_port) != 1) {
			fprintf(stderr, "Invalid --port argument\n\n");
			return false;
		}
		return true;
	case 'c':
		if (!optarg) {
			fprintf(stderr, "Invalid --contenturl argument\n\n");
			return false;
		}
		set_content_url(optarg);
		return true;
	case 'v':
		if (!optarg) {
			fprintf(stderr, "Invalid --versionurl argument\n\n");
			return false;
		}
		set_version_url(optarg);
		return true;
	case 'F':
		if (!optarg || !set_format_string(optarg)) {
			fprintf(stderr, "Invalid --format argument\n\n");
			return false;
		}
		return true;
	case 'S':
		if (!optarg || !set_state_dir(optarg)) {
			fprintf(stderr, "Invalid --statedir argument\n\n");
			return false;
		}
		return true;
	case 'n':
		sigcheck = false;
		return true;
	case 'I':
		timecheck = false;
		return true;
	case 't':
		verbose_time = true;
		return true;
	case 'N':
		no_scripts = true;
		return true;
	case 'b':
		no_boot_update = true;
		return true;
	case 'C':
		if (!optarg) {
			fprintf(stderr, "Invalid --certpath argument\n\n");
			return false;
		}
		set_cert_path(optarg);
		return true;
	case 'W':
		if (sscanf(optarg, "%d", &max_parallel_downloads) != 1) {
			fprintf(stderr, "Invalid --max-parallel-downloads argument\n\n");
			return false;
		}
		return true;
	}

	return false;
}

static bool global_parse_deprecated(int opt, char *optarg)
{
	switch (opt) {
	case 'D':
		fprintf(stderr, "Deprecated option -D was renamed. Prefer using -W or --max-parallel-downloads.\n\n");
		return global_parse_opt('W', optarg);
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
		opts++;
	}
	optstring[i] = '\0';

	return optstring;
}

void global_print_help(void)
{
	fprintf(stderr, "Global Options:\n");
	fprintf(stderr, "   -h, --help              Show help options\n");
	fprintf(stderr, "   -p, --path=[PATH...]    Use [PATH...] as the path to verify (eg: a chroot or btrfs subvol\n");
	fprintf(stderr, "   -u, --url=[URL]         RFC-3986 encoded url for version string and content file downloads\n");
	fprintf(stderr, "   -P, --port=[port #]     Port number to connect to at the url for version string and content file downloads\n");
	fprintf(stderr, "   -c, --contenturl=[URL]  RFC-3986 encoded url for content file downloads\n");
	fprintf(stderr, "   -v, --versionurl=[URL]  RFC-3986 encoded url for version file downloads\n");
	fprintf(stderr, "   -F, --format=[staging,1,2,etc.]  the format suffix for version file downloads\n");
	fprintf(stderr, "   -x, --force             Attempt to proceed even if non-critical errors found\n");
	fprintf(stderr, "   -n, --nosigcheck        Do not attempt to enforce certificate or signature checking\n");
	fprintf(stderr, "   -I, --ignore-time       Ignore system/certificate time when validating signature\n");
	fprintf(stderr, "   -S, --statedir          Specify alternate swupd state directory\n");
	fprintf(stderr, "   -C, --certpath          Specify alternate path to swupd certificates\n");
	fprintf(stderr, "   -t, --time              Show verbose time output for swupd operations\n");
	fprintf(stderr, "   -N, --no-scripts        Do not run the post-update scripts and boot update tool\n");
	fprintf(stderr, "   -b, --no-boot-update    Do not install boot files to the boot partition (containers)\n");
	fprintf(stderr, "   -W, --max-parallel-downloads=[n] Set the maximum number of parallel downloads\n");
	fprintf(stderr, "\n");
}

int global_parse_options(int argc, char **argv, const struct global_options *opts)
{
	struct option *opts_array;
	int num_global_opts, opt;
	char *optstring;
	int ret = 0;

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

		// Try to parse deprecated global options
		if (global_parse_deprecated(opt, optarg)) {
			continue;
		}

		ret = -1;
		goto end;
	}

end:
	free(optstring);
	free(opts_array);
	return ret ? ret : optind;
}
