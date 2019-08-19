/*
 *   Software Updater - client side
 *
 *      Copyright © 2018 Intel Corporation.
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
 */

#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "swupd.h"

static bool set = false;
static bool unset = false;
const char *set_url = NULL;

static void print_help(void)
{
	print("Usage:\n");
	print("   swupd mirror [OPTION...]\n\n");

	//TODO: Add documentation explaining this command

	global_print_help();

	print("Options:\n");
	print("   -s, --set [URL]         set mirror url\n");
	print("   -U, --unset             unset mirror url\n");
	print("\n");
}

static const struct option prog_opts[] = {
	{ "set", no_argument, 0, 's' },
	{ "unset", no_argument, 0, 'U' },
};

static bool parse_opt(int opt, UNUSED_PARAM char *optarg)
{
	switch (opt) {
	case 's':
		if (unset) {
			error("cannot set and unset at the same time\n");
			return false;
		}
		set = true;
		return true;
	case 'U':
		if (set) {
			error("cannot set and unset at the same time\n");
			return false;
		}
		unset = true;
		return true;
	default:
		return false;
	}
	return false;
}

static const struct global_options opts = {
	prog_opts,
	sizeof(prog_opts) / sizeof(struct option),
	parse_opt,
	print_help,
};

static bool parse_options(int argc, char **argv)
{
	int ind = global_parse_options(argc, argv, &opts);

	if (ind < 0) {
		return false;
	}

	if (!set && (globals.content_url || globals.version_url)) {
		error("Version or content url can only be used with --set\n");
		return false;
	}

	if (argc <= ind) {
		// Check if URLs are set
		if (set && !globals.content_url && !globals.version_url) {
			error("Option '--set' requires an argument\n");
			return false;
		}
		return true;
	} else if (set && argc == ind + 1) {
		set_url = argv[ind];
		return true;
	}

	error("Unexpected arguments\n\n");
	return false;
}

static int unset_mirror_url()
{
	char *content_path;
	char *version_path;
	int ret1 = 0, ret2 = 0;
	content_path = mk_full_filename(globals.path_prefix, MIRROR_CONTENT_URL_PATH);
	version_path = mk_full_filename(globals.path_prefix, MIRROR_VERSION_URL_PATH);

	ret1 = swupd_rm(content_path);
	ret2 = swupd_rm(version_path);

	free_string(&content_path);
	free_string(&version_path);

	// No errors
	if ((ret1 == -ENOENT && !ret2) ||
	    (ret2 == -ENOENT && !ret1)) {
		return 0;
	}

	if (ret1) {
		return ret1;
	}
	return ret2;
}

static enum swupd_code write_to_path(const char *content, const char *path)
{
	char *dir, *tmp = NULL;
	struct stat dirstat;
	string_or_die(&tmp, "%s", path);
	dir = dirname(tmp);

	/* attempt to make the directory, ok if already exists */
	int ret = mkdir(dir, S_IRWXU | S_IRWXG | S_IRWXO);
	if (ret && EEXIST != errno) {
		error("failed to create directory %s\n", dir);
		ret = SWUPD_COULDNT_CREATE_DIR;
		goto out;
	}

	/* Make sure we have a directory, for better user feedback */
	if ((ret = stat(dir, &dirstat))) {
		error("%s: failed to create directory\n", dir);
		ret = SWUPD_COULDNT_CREATE_DIR;
		goto out;
	} else if (!S_ISDIR(dirstat.st_mode)) {
		error("%s: not a directory\n", dir);
		ret = SWUPD_COULDNT_CREATE_DIR;
		goto out;
	}

	/* now try to open the file to write */
	FILE *fp = NULL;
	fp = fopen(path, "w");
	if (fp == NULL) {
		error("%s: failed to open file\n", path);
		ret = SWUPD_COULDNT_WRITE_FILE;
		goto out;
	}

	/* and write to the file */
	ret = fputs(content, fp);
	if (ret < 0 || ret == EOF) {
		ret = SWUPD_COULDNT_WRITE_FILE;
		error("%s: write failed\n", path);
	}

	if ((ret = fclose(fp))) {
		ret = SWUPD_COULDNT_WRITE_FILE;
		error("%s: failed to close file\n", path);
	}

out:
	free_string(&tmp);
	return ret;
}

static enum swupd_code set_new_url(const char *url, const char *url_path)
{
	int ret;
	char *path;

	/* enforce the use of https */
	if (!is_url_allowed(url)) {
		return SWUPD_INVALID_OPTION;
	}

	/* concatenate path_prefix and configuration paths if necessary
	 * if path_prefix is NULL the second argument will be returned */
	path = mk_full_filename(globals.path_prefix, url_path);

	/* write url to path_prefix/MIRROR_VERSION_URL_PATH */
	ret = write_to_path(url, path);
	free(path);

	return ret;
}

static enum swupd_code set_mirror_url(const char *url_new)
{
	int ret;
	const char *content_url_new = globals.content_url;
	const char *version_url_new = globals.version_url;

	if (url_new) {
		content_url_new = url_new;
		version_url_new = url_new;
	}

	if (version_url_new) {
		ret = set_new_url(version_url_new, MIRROR_VERSION_URL_PATH);
		if (ret != SWUPD_OK) {
			return ret;
		}
	}

	if (content_url_new) {
		ret = set_new_url(content_url_new, MIRROR_CONTENT_URL_PATH);
		if (ret != SWUPD_OK) {
			return ret;
		}
	}

	if (is_url_insecure(content_url_new) || is_url_insecure(version_url_new)) {
		warn("The mirror was set up using HTTP. In order for autoupdate "
		     "to continue working you will need to set allow_insecure_http=true "
		     "in the swupd configuration file. Alternatively you can set the "
		     "mirror using HTTPS (recommended)\n\n");
	}

	return SWUPD_OK;
}

static bool mirror_is_set(void)
{
	bool mirror_set;
	char *version_path;
	char *content_path;

	version_path = mk_full_filename(globals.path_prefix, MIRROR_VERSION_URL_PATH);
	content_path = mk_full_filename(globals.path_prefix, MIRROR_CONTENT_URL_PATH);
	mirror_set = file_exists(version_path) && file_exists(content_path);

	free_string(&version_path);
	free_string(&content_path);

	return mirror_set;
}

int handle_mirror_if_stale(void)
{
	int ret = 0;
	char *ret_str = NULL;
	char *fullpath = NULL;

	/* make sure there is a mirror set up first */
	if (!mirror_is_set()) {
		return 0;
	}

	info("Checking mirror status\n");

	fullpath = mk_full_filename(globals.path_prefix, DEFAULT_VERSION_URL_PATH);
	ret = get_value_from_path(&ret_str, fullpath, true);
	if (ret != 0 || ret_str == NULL) {
		/* no versionurl file here, might not exist under --path argument */
		goto out;
	}

	/* before trying to get the latest version let's make sure the central version is up */
	if (!globals.content_url_is_local && check_connection(ret_str) != 0) {
		warn("Upstream server %s not responding, cannot determine upstream version\n", ret_str);
		warn("Unable to determine if the mirror is up to date\n");
		goto out;
	}

	int central_version = get_latest_version(ret_str);
	int mirror_version = get_latest_version("");

	/* if the latest version could not be retrieved from the central server we cannot
	 * check if mirror is stale, this would be very odd since we already check connection
	 * with the server earlier */
	if (central_version < 0) {
		warn("Upstream server %s not responding, cannot determine upstream version\n", ret_str);
		warn("Unable to determine if the mirror is up to date\n");
		goto out;
	}

	/* if the mirror_version has an error or it's outdated and central_version works,
	 * we should unset the mirror*/
	int diff = central_version - mirror_version;
	if (mirror_version > 0 && diff <= MIRROR_STALE_UNSET) {
		if (diff > MIRROR_STALE_WARN) {
			warn("mirror version (%d) is behind upstream version (%d)\n",
			     mirror_version,
			     central_version);
		}
		/* no need to unset */
		goto out;
	}

	/* if we've made it this far we need to unset */
	if (mirror_version < 0) {
		warn("the mirror version could not be determined\n");
	} else {
		warn("the mirror version (%d) is too far behind upstream version (%d)\n",
		     mirror_version,
		     central_version);
	}

	info("Removing mirror configuration\n");
	unset_mirror_url();
	if (!set_default_urls()) {
		ret = -1;
		goto out;
	}
	get_latest_version("");

out:
	free_string(&fullpath);
	free_string(&ret_str);
	return ret;
}

/* return 0 if update available, non-zero if not */
enum swupd_code mirror_main(int argc, char **argv)
{
	int ret = SWUPD_OK;
	enum swupd_code init_ret;
	const int steps_in_mirror = 1;

	/* there is no need to report in progress for mirror at this time */

	if (!parse_options(argc, argv)) {
		print_help();
		return SWUPD_INVALID_OPTION;
	}
	progress_init_steps("mirror", steps_in_mirror);

	if (set) {
		check_root();
		ret = set_mirror_url(set_url);
		if (ret != SWUPD_OK) {
			error("Unable to set mirror url\n");
			goto finish;
		} else {
			print("Mirror url set\n");
		}
	} else if (unset) {
		check_root();
		ret = unset_mirror_url();
		if (ret == -ENOENT) {
			info("No mirror url configuration to remove\n");
			ret = SWUPD_OK;
		} else if (ret != 0) {
			ret = SWUPD_COULDNT_REMOVE_FILE;
			error("Unable to remove mirror configuration\n");
		} else { /* ret == 0 */
			print("Mirror url configuration removed\n");
		}
	}

	/* init swupd here after the new URL is configured */
	init_ret = swupd_init(SWUPD_NO_NETWORK | SWUPD_NO_TIMECHECK | SWUPD_NO_ROOT);
	if (init_ret != SWUPD_OK) {
		ret = init_ret;
		goto finish;
	}

	/* print new configuration */
	print_update_conf_info();
	swupd_deinit();

finish:
	progress_finish_steps("mirror", ret);
	return ret;
}
