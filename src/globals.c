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
#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "swupd.h"

bool force = false;
bool sigcheck = true;
bool verify_esp_only;
bool verify_bundles_only = false;
int update_count = 0;
int update_skip = 0;
bool need_update_boot = false;
bool need_update_bootloader = false;
bool update_complete = false;
#if 0
/* disabled unused global variables */
bool ignore_config = true;
bool ignore_state = true;
#endif
bool ignore_orphans = true;
char *format_string = NULL;
char *path_prefix = NULL; /* must always end in '/' */
char *mounted_dirs = NULL;
char *bundle_to_add = NULL;
struct timeval start_time;
char *state_dir = NULL;

/* NOTE: Today the content and version server urls are the same in
 * all cases.  It is highly likely these will eventually differ, eg:
 * swupd-version.01.org and swupd-files.01.org as this enables
 * different quality of server and control of the servers
 */
bool download_only;
bool local_download = false;
bool have_manifest_diskspace = false; /* assume no until checked */
bool have_network = false;	    /* assume no access until proved */
char *version_url = NULL;
char *content_url = NULL;
long update_server_port = -1;

static const char *default_version_url_path = "/usr/share/defaults/swupd/versionurl";
static const char *default_content_url_path = "/usr/share/defaults/swupd/contenturl";
static const char *default_format_path = "/usr/share/defaults/swupd/format";

static int set_default_value_from_path(char **global, const char *path)
{
	char line[LINE_MAX];
	FILE *file;
	char *c;
	char *rel_path;
	int ret = -1;

	string_or_die(&rel_path, "%s%s", path_prefix, path);

	file = fopen(rel_path, "r");
	if (!file) {
		free(rel_path);
		return ret;
	}

	/* the file should contain exactly one line */
	line[0] = 0;
	if (fgets(line, LINE_MAX, file) == NULL) {
		if (ferror(file)) {
			printf("Error: Unable to read data from %s\n", rel_path);
		} else if (feof(file)) {
			printf("Error: Contents of %s are empty\n", rel_path);
		}
		goto fail;
	}

	/* remove newline if present */
	c = strchr(line, '\n');
	if (c) {
		*c = '\0';
	}

	string_or_die(global, "%s", line);
	ret = 0;
fail:
	fclose(file);
	free(rel_path);
	return ret;
}

static int set_url(char **global, char *url, const char *path)
{
	int ret = 0;

	if (url) {
		if (*global) {
			free(*global);
		}
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

int set_content_url(char *url)
{
	if (content_url) {
		/* Only set once; we assume the first successful set is the best choice */
		return 0;
	}

	return set_url(&content_url, url, default_content_url_path);
}

int set_version_url(char *url)
{
	if (version_url) {
		/* Only set once; we assume the first successful set is the best choice */
		return 0;
	}

	return set_url(&version_url, url, default_version_url_path);
}

static bool is_valid_integer_format(char *str)
{
	unsigned long long int version;
	errno = 0;

	version = strtoull(str, NULL, 10);
	if ((errno < 0) || (version == 0)) {
		return false;
	}

	return true;
}

bool set_state_dir(char *path)
{
	if (path) {
		if (path[0] != '/') {
			printf("statepath must be a full path starting with '/', not '%c'\n", path[0]);
			return false;
		}

		if (state_dir) {
			free(state_dir);
		}

		string_or_die(&state_dir, "%s", path);
	} else {
		if (state_dir) {
			return true;
		}
		string_or_die(&state_dir, "%s", STATE_DIR);
	}

	return true;
}

bool set_format_string(char *userinput)
{
	int ret;

	if (format_string) {
		return true;
	}

	if (userinput) {
		// allow "staging" as a format string
		if ((strcmp(userinput, "staging") == 0)) {
			if (format_string) {
				free(format_string);
			}
			string_or_die(&format_string, "%s", userinput);
			return true;
		}

		// otherwise, expect a positive integer
		if (!is_valid_integer_format(userinput)) {
			return false;
		}
		if (format_string) {
			free(format_string);
		}
		string_or_die(&format_string, "%s", userinput);
	} else {
		/* no option passed; use the default value */
		ret = set_default_value_from_path(&format_string, default_format_path);
		if (ret < 0) {
			return false;
		}
		if (!is_valid_integer_format(format_string)) {
			return false;
		}
	}

	return true;
}

/* Passing NULL for PATH will either use the last --path argument given on the command
 * line, or sets the default value ("/").
 */
bool set_path_prefix(char *path)
{
	struct stat statbuf;
	int ret;

	if (path != NULL) {
		int len;
		char *tmp;

		/* in case multiple -p options are passed */
		if (path_prefix) {
			free(path_prefix);
		}

		string_or_die(&tmp, "%s", path);

		/* ensure path_prefix is absolute, at least '/', ends in '/',
		 * and is a valid dir */
		if (tmp[0] != '/') {
			char *cwd;

			cwd = get_current_dir_name();
			if (cwd == NULL) {
				printf("Unable to getwd() (%s)\n", strerror(errno));
				free(tmp);
				return false;
			}

			free(tmp);
			string_or_die(&tmp, "%s/%s", cwd, path);

			free(cwd);
		}

		len = strlen(tmp);
		if (!len || (tmp[len - 1] != '/')) {
			char *tmp_old = tmp;
			string_or_die(&tmp, "%s/", tmp_old);
			free(tmp_old);
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
		printf("Bad path_prefix %s (%s), cannot continue.\n",
		       path_prefix, strerror(errno));
		return false;
	}

	return true;
}

bool init_globals(void)
{
	int ret;

	gettimeofday(&start_time, NULL);

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
		printf("Unable to determine format id. Use the -F option instead.\n");
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
			printf("\nDefault version URL not found. Use the -v option instead.\n");
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
			printf("\nDefault content URL not found. Use the -c option instead.\n");
			exit(EXIT_FAILURE);
		}
	}

	/* must set this global after version_url and content_url */
	set_local_download();

	return true;
}

void free_globals(void)
{
	free(content_url);
	free(version_url);
	free(path_prefix);
	free(format_string);
	free(mounted_dirs);
	free(state_dir);
	if (bundle_to_add != NULL) {
		free(bundle_to_add);
	}
}
