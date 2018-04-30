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

static void print_help(const char *name)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "   swupd %s [options] [mirror_url]\n\n", basename((char *)name));
	fprintf(stderr, "Help Options:\n");
	fprintf(stderr, "   -h, --help              Show help options\n");
	fprintf(stderr, "   -s, --set               set mirror url\n");
	fprintf(stderr, "   -u, --unset             unset mirror url\n");
	fprintf(stderr, "   -p, --path=[PATH...]    Use [PATH...] as the path to set/unset the mirror url for (eg: a chroot or btrfs subvol\n");
	fprintf(stderr, "\n");
}

static const struct option prog_opts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "set", required_argument, 0, 's' },
	{ "unset", no_argument, 0, 'u' },
	{ "path", required_argument, 0, 'p' },
	{ 0, 0, 0, 0 }
};

static int unset_mirror_url()
{
	char *fullpath;
	if (path_prefix != NULL) {
		fullpath = mk_full_filename(path_prefix, "/etc/swupd");
	} else {
		string_or_die(&fullpath, "/etc/swupd");
	}

	if (fullpath == NULL ||
	    strcmp(fullpath, "/") == 0 ||
	    strcmp(fullpath, "/etc") == 0) {
		fprintf(stderr, "Invalid mirror configuration path\n");
		free_string(&fullpath);
		return 1;
	}
	int ret = swupd_rm(fullpath);
	free_string(&fullpath);
	return ret;
}

static int write_to_path(char *content, char *path)
{
	char *dir, *tmp = NULL;
	string_or_die(&tmp, "%s", path);
	dir = dirname(tmp);
	/* attempt to  make the directory
	 * ignore EEXIST errors */
	int ret = mkdir(dir, S_IRWXU);
	char *cmd;
	string_or_die(&cmd, "mkdir -p %s", dir);
	ret = system(cmd);
	free_string(&cmd);
	free_string(&tmp);
	if (ret) {
		return ret;
	}

	/* now try to open the file to write */
	FILE *fp = NULL;
	fp = fopen(path, "w");
	if (fp == NULL) {
		return 1;
	}

	/* and write to the file */
	ret = fputs(content, fp);
	fclose(fp);
	if (ret < 0 || ret == EOF) {
		return 1;
	}

	return 0;
}

static int set_mirror_url(char *url)
{
	char *content_path;
	char *version_path;
	int ret = 0;
	bool need_unset = false;
	/* concatenate path_prefix and configuration paths if necessary */
	if (path_prefix != NULL) {
		content_path = mk_full_filename(path_prefix, MIRROR_CONTENT_URL_PATH);
		version_path = mk_full_filename(path_prefix, MIRROR_VERSION_URL_PATH);
	} else {
		string_or_die(&content_path, "%s", MIRROR_CONTENT_URL_PATH);
		string_or_die(&version_path, "%s", MIRROR_VERSION_URL_PATH);
	}

	/* write url to path_prefix/MIRROR_CONTENT_URL_PATH */
	ret = write_to_path(url, content_path);
	if (ret != 0) {
		need_unset = true;
		goto out;
	}

	/* write url to path_prefix/MIRROR_VERSION_URL_PATH */
	ret = write_to_path(url, version_path);
	if (ret != 0) {
		need_unset = true;
		goto out;
	}

out:
	if (need_unset) {
		unset_mirror_url();
	}
	free_string(&content_path);
	free_string(&version_path);
	return ret;
}

void handle_mirror_if_stale(void)
{
	char *ret_str = NULL;
	char *fullpath = NULL;

	if (path_prefix != NULL) {
		fullpath = mk_full_filename(path_prefix, DEFAULT_VERSION_URL_PATH);
	} else {
		string_or_die(&fullpath, DEFAULT_VERSION_URL_PATH);
	}
	int ret = get_value_from_path(&ret_str, fullpath, false);
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
		fprintf(stderr, "WARNING: removing stale mirror configuration. "
				"Mirror version (%d) is too far behind upstream version (%d)\n",
			mirror_version,
			central_version);
		unset_mirror_url();
		goto out;
	}
	if (diff > MIRROR_STALE_WARN) {
		fprintf(stderr, "WARNING: mirror version (%d) is behind upstream version (%d)\n",
			mirror_version,
			central_version);
	}
out:
	free_string(&fullpath);
	free_string(&ret_str);
}

char *set = NULL;
bool unset = false;

static int parse_options(int argc, char **argv)
{
	int opt, ret = 0;

	while ((opt = getopt_long(argc, argv, "hs:up:", prog_opts, NULL)) != -1) {
		switch (opt) {
		case '?':
			ret = EINVALID_OPTION;
			goto out;
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'p':
			if (!set_path_prefix(optarg)) {
				fprintf(stderr, "Invalid --path argument\n\n");
				ret = EINVALID_OPTION;
				goto out;
			}
			break;
		case 's':
			if (unset) {
				fprintf(stderr, "cannot set and unset at the same time\n");
				ret = EINVALID_OPTION;
				goto out;
			}
			set = optarg;
			break;
		case 'u':
			if (set != NULL) {
				fprintf(stderr, "cannot set and unset at the same time\n");
				ret = EINVALID_OPTION;
				goto out;
			}
			unset = true;
			break;
		default:
			fprintf(stderr, "error: unrecognized option\n\n");
			ret = EINVALID_OPTION;
			goto out;
		}
	}

out:
	if (ret == EINVALID_OPTION) {
		print_help(argv[0]);
	}
	return ret;
}

/* return 0 if update available, non-zero if not */
int mirror_main(int argc, char **argv)
{
	int ret = 0;
	copyright_header("mirror");
	ret = parse_options(argc, argv);
	if (ret != 0) {
		return ret;
	}

	if (set != NULL) {
		ret = set_mirror_url(set);
		if (ret != 0) {
			fprintf(stderr, "Unable to set mirror url\n");
		} else {
			fprintf(stderr, "Set upstream mirror to %s\n", set);
		}
	} else if (unset) {
		ret = unset_mirror_url();
		if (ret == -ENOENT) {
			fprintf(stderr, "No mirror url configuration to remove\n");
			ret = 0;
		} else if (ret != 0) {
			fprintf(stderr, "Unable to remove mirror configuration\n");
		} else { /* ret == 0 */
			fprintf(stderr, "Mirror url configuration removed\n");
		}
	}

	/* init globals here after the new URL is configured */
	if (!init_globals()) {
		return EINIT_GLOBALS;
	}

	/* print new configuration */
	print_update_conf_info();
	free_globals();
	return ret;
}
