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
#include "config.h"
#include "swupd.h"
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <unistd.h>

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
bool fix = false;
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
bool have_manifest_diskspace = false; /* assume no until checked */
bool have_network = false;	    /* assume no access until proved */
#define URL_COUNT 2
char *version_server_urls[URL_COUNT] = {
	NULL,
	"https://download.clearlinux.org/update",
};
char *content_server_urls[URL_COUNT] = {
	NULL,
	"https://download.clearlinux.org/update",
};
char *preferred_version_url;
char *preferred_content_url;
long update_server_port = -1;

#define SWUPD_DEFAULT_FORMAT "3"
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

	/* pick urls simply from user specified or default */
	if (version_server_urls[0] != NULL) {
		preferred_version_url = version_server_urls[0];
	} else {
		preferred_version_url = version_server_urls[1];
	}
	if (content_server_urls[0] != NULL) {
		preferred_content_url = content_server_urls[0];
	} else {
		preferred_content_url = content_server_urls[1];
	}

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
	free(content_server_urls[0]);
	free(version_server_urls[0]);
	free(path_prefix);
	free(format_string);
	free(mounted_dirs);
	if (bundle_to_add != NULL) {
		free(bundle_to_add);
	}
}
