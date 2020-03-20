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
 *         Icarus Sparry <icarus.w.sparry@intel.com>
 *
 */

/* This file implements the "swupd verify --picky" command.
 * This finds all the files under $root/usr which are not
 * listed in the current manifest except those in /lib/modules
 * and /usr/local
 */

#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "signature.h"
#include "swupd.h"

static struct filerecord *F = NULL; /* Array of filerecords */
static int nF = 0;		    /* Number of filerecords */

static const regex_t *path_whitelist;
static int path_prefix_len;

static int record_filename(const char *name, const struct stat *stat __attribute__((unused)), int type, struct FTW *ftw __attribute__((unused)))
{
	/* Name as it would appear in manifest, f.i. /usr */
	const char *relname = name + path_prefix_len - 1;

	if (!relname[0] || !strcmp(relname, "/")) {
		/* Do not record root while descending into it. */
		return 0;
	}

	if (path_whitelist) {
		int match_res;
		match_res = regexec(path_whitelist, relname, 0, NULL, 0);
		if (match_res == 0) {
			/* ignore matching entry and everything underneath it */
			return FTW_SKIP_SUBTREE;
		}
	}

	char *savedname = strdup_or_die(relname); /* Only store name relative to top of area */
	F = realloc(F, (nF + 1) * sizeof(*F));	  /* TODO, check realloc is smart, so don't need to double myself */
	ON_NULL_ABORT(F);

	F[nF].filename = savedname;
	F[nF].dir = (type == FTW_D);
	F[nF].in_manifest = false; /* Because we do yet know */
	nF++;
	return 0;
}

/* return true if the function deletes the specified filename, otherwise false */
static bool handle(const char *filename, bool is_dir, bool fix)
{
	char *temp;
	bool ret = true;

	temp = sys_path_join("%s/%s", globals.path_prefix, filename);
	if (fix) {
		print(" -> Extra file: %s%s", temp, is_dir ? "/" : "");
		if (remove(temp)) {
			print(" -> not deleted (%s)\n", strerror(errno));
			ret = false;
		} else {
			print(" -> deleted\n");
		}
	} else {
		print(" -> Extra file: %s%s\n", temp, is_dir ? "/" : "");
		ret = false;
	}
	free_and_clear_pointer(&temp);

	return ret;
}

/* expect the start to end in /usr and be the absolute path to the root */
enum swupd_code walk_tree(struct manifest *manifest, const char *start, bool fix, const regex_t *whitelist, struct file_counts *counts)
{
	/* Walk the tree, */
	int rc;
	int ret;

	path_prefix_len = strlen(globals.path_prefix);
	path_whitelist = whitelist;
	rc = nftw(start, &record_filename, 0, FTW_ACTIONRETVAL | FTW_PHYS | FTW_MOUNT);
	const char *skip_dir = NULL; /* Skip files below this in printout */
	if (rc) {
		if (errno == ENOENT) {
			rc = SWUPD_COULDNT_LIST_DIR;
		} else {
			rc = SWUPD_OUT_OF_MEMORY_ERROR;
		}
		goto tidy; /* Already printed out of memory */
	}
	qsort(F, nF, sizeof(*F), &cmp_filerecord_filename);
	/* Interesting question, would it be faster to sort this linked list,
	 * or convert it to an array of pointers, or just pull them off one
	 * at a time? Try one at a time first.
	 */
	struct list *iter = list_head(manifest->files);
	while (iter) {
		struct file *file;
		struct filerecord *found;

		file = iter->data;
		iter = iter->next;

		if (file->is_deleted && !file->is_ghosted) {
			continue;
		}
		found = bsearch(&file->filename, F, nF, sizeof(*F), &cmp_string_filerecord_filename);
		if (found) {
			found->in_manifest = true;
		}
	}

	int skip_len = 0; /* Length of directory name we are skipping
			   * could have used strlen(skip_dir), but speed! */
	/* list files/directories which are extra.
	 * This is reverse so that files are removed before their parent dirs */
	for (int i = nF - 1; i >= 0; i--) {
		if (!F[i].in_manifest) {
			/* Account for these files not in the manifest as inspected also */
			counts->checked++;
			counts->extraneous++;
			counts->picky_extraneous++;
			/* Logic to avoid printing out all the files in a
			 * directory when the directory itself is not present */
			if (skip_dir) {
				/* This is under the current directory to be skipped */
				if ((strncmp(F[i].filename, skip_dir, skip_len) == 0) &&
				    F[i].filename[skip_len] == '/') {
					continue;
				}
			}
			if (F[i].dir) { /* Start of new dir to skip */
				skip_dir = F[i].filename;
				skip_len = strlen(skip_dir);
				ret = handle(F[i].filename, true, fix);
			} else {
				ret = handle(F[i].filename, false, fix);
			}
			if (ret) {
				counts->deleted++;
			} else {
				counts->not_deleted++;
			}
		} else {
			skip_dir = NULL;
		}
	}
tidy:
	for (int i = 0; i < nF; i++) {
		free_and_clear_pointer(&F[i].filename);
	}
	free(F);
	F = NULL;
	nF = 0;

	return rc;
}
