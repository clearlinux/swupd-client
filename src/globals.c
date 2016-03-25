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

#define SWUPD_DEFAULT_FORMAT "3"
static const char *default_version_url_path = "/usr/share/defaults/swupd/versionurl";
static const char *default_content_url_path = "/usr/share/defaults/swupd/contenturl";

static int set_default_value(char **global, const char *path)
{
	char line[LINE_MAX];
	FILE *file;
	char *c;

	file = fopen(path, "r");
	if (!file) {
		printf("Error: Unable to open %s\n", path);
		return -1;
	}

	/* the file should contain exactly one line */
	line[0] = 0;
	if (fgets(line, LINE_MAX, file) == NULL) {
		if (ferror(file)) {
			printf("Error: Unable to read data from %s\n", path);
			return -1;
		}
		if (feof(file)) {
			printf("Error: Contents of %s are empty\n", path);
			return -1;
		}
	}

	/* remove newline if present */
	c = strchr(line, '\n');
	if (c) {
		*c = '\0';
	}

	string_or_die(global, "%s", line);

	return 0;
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
			ret = set_default_value(global, path);
			if (ret < 0) {
				return ret;
			}
		}
	}

	return ret;
}

void set_content_url(char *url) {
	int ret;

	ret = set_url(&content_url, url, default_content_url_path);
	if (ret < 0) {
		printf("\nDefault content URL cannot be read. Use the -c option instead.\n");
		exit(EXIT_FAILURE);
	}
}

void set_version_url(char *url) {
	int ret;

	ret = set_url(&version_url, url, default_version_url_path);
	if (ret < 0) {
		printf("\nDefault version URL cannot be read. Use the -v option instead.\n");
		exit(EXIT_FAILURE);
	}
}

bool set_format_string(char *userinput)
{
	int version;

	if (userinput == NULL) {
		if (format_string) {
			free(format_string);
		}
		string_or_die(&format_string, "%s", SWUPD_DEFAULT_FORMAT);
		return true;
	}

	// allow "staging" as a format string
	if ((strcmp(userinput, "staging") == 0)) {
		if (format_string) {
			free(format_string);
		}
		string_or_die(&format_string, "%s", userinput);
		return true;
	}

	// otherwise, expect a positive integer
	errno = 0;
	version = strtoull(userinput, NULL, 10);
	if ((errno < 0) || (version <= 0)) {
		return false;
	}
	if (format_string) {
		free(format_string);
	}
	string_or_die(&format_string, "%d", version);

	return true;
}

bool init_globals(void)
{
	struct stat statbuf;
	int ret;

	gettimeofday(&start_time, NULL);

	set_version_url(NULL);
	set_content_url(NULL);

	/* insure path_prefix is absolute, at least '/', ends in '/',
	 * and is a valid dir */
	if (path_prefix != NULL) {
		int len;
		char *tmp;

		if (path_prefix[0] != '/') {
			char *cwd;

			cwd = get_current_dir_name();
			if (cwd == NULL) {
				printf("Unable to getwd() (%s)\n", strerror(errno));
				return false;
			}
			string_or_die(&tmp, "%s/%s", cwd, path_prefix);

			free(path_prefix);
			path_prefix = tmp;
			free(cwd);
		}

		len = strlen(path_prefix);
		if (!len || (path_prefix[len - 1] != '/')) {
			string_or_die(&tmp, "%s/", path_prefix);
			free(path_prefix);
			path_prefix = tmp;
		}
	} else {
		string_or_die(&path_prefix, "/");
	}
	ret = stat(path_prefix, &statbuf);
	if (ret != 0 || !S_ISDIR(statbuf.st_mode)) {
		printf("Bad path_prefix %s (%s), cannot continue.\n",
		       path_prefix, strerror(errno));
		return false;
	}

	return true;
}

void free_globals(void)
{
	free(content_url);
	free(version_url);
	free(path_prefix);
	free(format_string);
	free(mounted_dirs);
	if (bundle_to_add != NULL) {
		free(bundle_to_add);
	}
}
