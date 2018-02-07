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
#include <bsdiff.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <linux/fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "swupd.h"
#include "xattrs.h"

static void do_delta(char *filename, struct file *file);

void try_delta(struct file *file)
{
	char *filename;
	struct stat stat;

	if (file->is_file == 0) {
		return;
	}

	if (file->deltapeer == NULL) {
		return;
	}

	if (file->deltapeer->is_file == 0) {
		return;
	}

	if (file->deltapeer->is_deleted) {
		return;
	}

	/* check if the full file is there already, because if it is, don't do the delta */
	string_or_die(&filename, "%s/staged/%s", state_dir, file->hash);
	if (lstat(filename, &stat) == 0) {
		free_string(&filename);
		return;
	}
	do_delta(filename, file);
	free_string(&filename);
}

static void do_delta(char *filename, struct file *file)
{
	char *origin;
	char *dir, *base, *tmp = NULL, *tmp2 = NULL;
	char *deltafile = NULL;
	int ret;
	struct stat sb;

	string_or_die(&deltafile, "%s/delta/%i-%i-%s-%s", state_dir,
		      file->deltapeer->last_change, file->last_change, file->deltapeer->hash, file->hash);

	ret = stat(deltafile, &sb);
	if (ret != 0) {
		free_string(&deltafile);
		return;
	}

	tmp = strdup(file->deltapeer->filename);
	tmp2 = strdup(file->deltapeer->filename);

	dir = dirname(tmp);
	base = basename(tmp2);

	string_or_die(&origin, "%s/%s/%s", path_prefix, dir, base);

	ret = apply_bsdiff_delta(origin, filename, deltafile);
	if (ret) {
		unlink_all_staged_content(file);
		goto out;
	}
	xattrs_copy(origin, filename);

	if (!verify_file(file, filename)) {
		unlink_all_staged_content(file);
		goto out;
	}

	if (xattrs_compare(origin, filename) != 0) {
		unlink_all_staged_content(file);
		goto out;
	}

	unlink(deltafile);

out:
	free_string(&origin);
	free_string(&deltafile);
	free_string(&tmp);
	free_string(&tmp2);
}
