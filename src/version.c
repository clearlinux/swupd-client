/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2016 Intel Corporation.
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
 *         Arjan van de Ven <arjan@linux.intel.com>
 *         Tim Pepper <timothy.c.pepper@linux.intel.com>
 *
 */

#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "swupd.h"

static int get_version_from_url(char *url)
{
#define MAX_VERSION_STR_SIZE 11

	int ret;
	int err;
	char version_str[MAX_VERSION_STR_SIZE];
	struct curl_file_data tmp_version = {
		MAX_VERSION_STR_SIZE, 0,
		version_str
	};

	ret = swupd_curl_get_file_memory(url, &tmp_version);
	if (ret) {
		goto out;
	} else {
		tmp_version.data[tmp_version.len] = '\0';
		err = strtoi_err(tmp_version.data, &ret);
		if (err != 0) {
			ret = -1;
		}
	}

out:
	return ret;
}

/* this function attempts to download the latest server version string file from
 * the preferred server to a memory buffer, returning either a negative integer
 * error code or >= 0 representing the server version
 *
 * if v_url is non-NULL the version at v_url is fetched. If v_url is NULL the
 * global version_url is used and the cached version may be used instead of
 * attempting to download the version string again. If v_url is the empty string
 * the global version_url is used and the cached version is ignored. */
int get_latest_version(char *v_url)
{
	char *url = NULL;
	int ret = 0;
	static int cached_version = -1;

	if (cached_version > 0 && v_url == NULL) {
		return cached_version;
	}

	if (v_url == NULL || strcmp(v_url, "") == 0) {
		v_url = globals.version_url;
	}

	string_or_die(&url, "%s/version/format%s/latest", v_url, globals.format_string);
	ret = get_version_from_url(url);
	free_string(&url);
	cached_version = ret;

	return ret;
}

/* gets the latest version of the update content regardless of what format we
 * are currently in */
int version_get_absolute_latest(void)
{
	char *url = NULL;
	int ret;

	string_or_die(&url, "%s/version/latest_version", globals.version_url);
	ret = get_version_from_url(url);
	free_string(&url);

	return ret;
}

/**
   Finds associated value string of the key in os-release file.
   Returns true for success, false otherwise.

   - path_prefix
   - key: 	Key in os-release file to read.
   - buff:	Buffer to store associated value.
 */
bool get_osrelease_value(char *path_prefix, char *key, char *buff)
{
	char line[LINE_MAX];
	FILE *file = NULL;
	char *releasefile = NULL;
	char *src = NULL, *dest = NULL;
	char *keystr = NULL;
	int keystrlen = 0;
	bool keyfound = false;

	string_or_die(&releasefile, "%s/usr/lib/os-release", path_prefix);
	file = fopen(releasefile, "rm");
	if (!file) {
		free_string(&releasefile);
		string_or_die(&releasefile, "%s/etc/os-release", path_prefix);
		file = fopen(releasefile, "rm");
		if (!file) {
			free_string(&releasefile);
			return false;
		}
	}

	string_or_die(&keystr, "%s=", key);
	keystrlen = strlen(keystr);
	while (!feof(file)) {
		line[0] = 0x00;
		if (fgets(line, LINE_MAX, file) == NULL) {
			break;
		}
		if (strncmp(line, keystr, keystrlen) == 0) {
			keyfound = true;
			src = &line[keystrlen];
			/* Drop quotes and newline in value */
			dest = buff;
			while (*src) {
				if (*src == '\'' || *src == '"' || *src == '\n') {
					++src;
				} else {
					*dest = *src;
					++dest;
					++src;
				}
			}
			*dest = 0;
			break;
		}
	}

	free_string(&releasefile);
	free_string(&keystr);
	fclose(file);
	return keyfound;
}

int get_current_version(char *path_prefix)
{
	char buff[LINE_MAX];
	int v = -1;

	if (!get_osrelease_value(path_prefix, "VERSION_ID", buff)) {
		return -1;
	}
	if (strtoi_err(buff, &v) != 0) {
		return -1;
	}

	return v;
}

/**
   Get distribution string from the os-release file.
   Returns true for success, false otherwise.

   - path_prefix: Prefix path for the os-release file.
   - dist: Buffer to store distribution string.
 */

bool get_distribution_string(char *path_prefix, char *dist)
{
	if (!get_osrelease_value(path_prefix, "PRETTY_NAME", dist)) {
		return false;
	}
	return true;
}

enum swupd_code read_versions(int *current_version, int *server_version, char *path_prefix, bool absolute)
{
	*current_version = get_current_version(path_prefix);
	if (absolute) {
		*server_version = version_get_absolute_latest();
	} else {
		*server_version = get_latest_version(NULL);
	}

	if (*current_version < 0) {
		error("Unable to determine current OS version\n");
		return SWUPD_CURRENT_VERSION_UNKNOWN;
	}
	if (*server_version < 0) {
		error("Unable to determine the server version\n");
		return SWUPD_SERVER_CONNECTION_ERROR;
	}

	return SWUPD_OK;
}

int read_mix_version_file(char *filename, char *path_prefix)
{
	char line[LINE_MAX];
	FILE *file;
	int v = -1;
	int err;
	char *buildstamp;

	string_or_die(&buildstamp, "%s%s", path_prefix, filename);
	file = fopen(buildstamp, "rm");
	if (!file) {
		free_string(&buildstamp);
		return v;
	}

	while (!feof(file)) {
		line[0] = 0;
		if (fgets(line, LINE_MAX, file) == NULL) {
			break;
		}

		/* Drop newline in value */
		char *c = strchr(line, '\n');
		if (c) {
			*c = '\0';
		}

		err = strtoi_err(line, &v);
		if (err != 0) {
			v = -1;
		}
	}
	free_string(&buildstamp);
	fclose(file);
	return v;
}

void check_mix_versions(int *current_version, int *server_version, char *path_prefix)
{
	*current_version = read_mix_version_file("/usr/share/clear/version", path_prefix);
	char *format_file;
	string_or_die(&format_file, MIX_STATE_DIR "version/format%s/latest", globals.format_string);
	*server_version = read_mix_version_file(format_file, path_prefix);
	free_string(&format_file);
}

int update_device_latest_version(int version)
{
	FILE *file = NULL;
	char *path = NULL;

	string_or_die(&path, "%s/version", globals.state_dir);
	file = fopen(path, "w");
	if (!file) {
		free_string(&path);
		return -1;
	}

	fprintf(file, "%i\n", version);
	fflush(file);
	fdatasync(fileno(file));
	fclose(file);
	free_string(&path);
	return 0;
}
