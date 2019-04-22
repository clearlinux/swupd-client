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

static char *set = NULL;
static bool unset = false;

static void print_help(void)
{
	print("Usage:\n");
	print("   swupd mirror [OPTION...]\n\n");

	//TODO: Add documentation explaining this command

	global_print_help();

	print("Options:\n");
	print("   -s, --set               set mirror url\n");
	print("   -u, --unset             unset mirror url\n");
	print("\n");
}

static const struct option prog_opts[] = {
	{ "set", required_argument, 0, 's' },
	{ "unset", no_argument, 0, 'u' },
};

static bool parse_opt(int opt, char *optarg)
{
	switch (opt) {
	case 's':
		if (unset) {
			error("cannot set and unset at the same time\n");
			return false;
		}
		set = optarg;
		return true;
	case 'u':
		if (set != NULL) {
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

	if (argc > ind) {
		error("unexpected arguments\n\n");
		return false;
	}

	return true;
}

static int unset_mirror_url()
{
	char *content_path;
	char *version_path;
	int ret = 0;
	content_path = mk_full_filename(path_prefix, MIRROR_CONTENT_URL_PATH);
	version_path = mk_full_filename(path_prefix, MIRROR_VERSION_URL_PATH);

	if ((ret = swupd_rm(content_path))) {
		goto out;
	}
	if ((ret = swupd_rm(version_path))) {
		goto out;
	}

out:
	free_string(&content_path);
	free_string(&version_path);
	return ret;
}

static enum swupd_code write_to_path(char *content, char *path)
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

static enum swupd_code set_mirror_url(char *url)
{
	char *content_path;
	char *version_path;
	int ret = SWUPD_OK;
	/* concatenate path_prefix and configuration paths if necessary
	 * if path_prefix is NULL the second argument will be returned */
	content_path = mk_full_filename(path_prefix, MIRROR_CONTENT_URL_PATH);
	version_path = mk_full_filename(path_prefix, MIRROR_VERSION_URL_PATH);

	/* write url to path_prefix/MIRROR_CONTENT_URL_PATH */
	ret = write_to_path(url, content_path);
	if (ret != SWUPD_OK) {
		goto out;
	}

	/* write url to path_prefix/MIRROR_VERSION_URL_PATH */
	ret = write_to_path(url, version_path);
	if (ret != SWUPD_OK) {
		goto out;
	}

out:
	free_string(&content_path);
	free_string(&version_path);
	return ret;
}

void handle_mirror_if_stale(void)
{
	char *ret_str = NULL;
	char *fullpath = NULL;

	fullpath = mk_full_filename(path_prefix, DEFAULT_VERSION_URL_PATH);
	int ret = get_value_from_path(&ret_str, fullpath, true);
	if (ret != 0 || ret_str == NULL) {
		/* no versionurl file here, might not exist under --path argument */
		goto out;
	}
	int central_version = get_latest_version(ret_str);
	int mirror_version = get_latest_version("");

	/* note that negative mirror_version and negative central_version are handled
	 * correctly, as the difference will be great for a mirror_version of -1
	 * (it will resolve to central_version + 1 and the diff will be very large).
	 * When the upstream central_version is an error -1 version the mirror version
	 * will be used due to the diff being negative. */
	int diff = central_version - mirror_version;
	if (diff > MIRROR_STALE_UNSET || mirror_version == -1) {
		warn("removing stale mirror configuration. "
		     "Mirror version (%d) is too far behind upstream version (%d)\n",
		     mirror_version,
		     central_version);
		unset_mirror_url();
		/* we need to re-set the cached_version to latest using the central version,
		 * since at the moment the cached_version is the outdated mirror version */
		free_string(&version_url);
		free_string(&content_url);
		set_version_url(NULL);
		set_content_url(NULL);
		get_latest_version(ret_str);
		goto out;
	}
	if (diff > MIRROR_STALE_WARN) {
		warn("mirror version (%d) is behind upstream version (%d)\n",
		     mirror_version,
		     central_version);
	}
out:
	free_string(&fullpath);
	free_string(&ret_str);
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

	if (set != NULL) {
		ret = set_mirror_url(set);
		if (ret != SWUPD_OK) {
			warn("Unable to set mirror url\n");
		} else {
			print("Set upstream mirror to %s\n", set);
		}
	} else if (unset) {
		ret = unset_mirror_url();
		if (ret == -ENOENT) {
			info("No mirror url configuration to remove\n");
			ret = SWUPD_OK;
		} else if (ret != 0) {
			ret = SWUPD_COULDNT_REMOVE_FILE;
			warn("Unable to remove mirror configuration\n");
		} else { /* ret == 0 */
			print("Mirror url configuration removed\n");
		}
	}

	/* init swupd here after the new URL is configured */
	init_ret = swupd_init(SWUPD_NO_NETWORK | SWUPD_NO_TIMECHECK);
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
