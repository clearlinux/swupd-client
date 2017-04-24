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
#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "swupd-build-variant.h"
#include "swupd.h"

/* clean then recreate temporary folder for tar renames */
static int create_staging_renamedir(char *rename_tmpdir)
{
	int ret;
	char *rmcommand = NULL;

	string_or_die(&rmcommand, "rm -fr %s", rename_tmpdir);
	if (!system(rmcommand)) {
		/* Not fatal but pretty scary, likely to really fail at the
		 * next command too. Pass for now as printing may just cause
		 * confusion */
		;
	}
	free(rmcommand);

	ret = mkdir(rename_tmpdir, S_IRWXU);
	if (ret == -1 && errno != EEXIST) {
		ret = -errno;
	} else {
		ret = 0;
	}

	return ret;
}

/* Do the staging of new files into the filesystem */
#warning "do_staging is currently not able to be run in parallel"
/* Consider adding a remove_leftovers() that runs in verify/fix in order to
 * allow this function to mkdtemp create folders for parallel build */
int do_staging(struct file *file, struct manifest *MoM)
{
	char *statfile = NULL, *tmp = NULL, *tmp2 = NULL;
	char *dir, *base, *rel_dir;
	char *tarcommand = NULL;
	char *original = NULL;
	char *target = NULL;
	char *targetpath = NULL;
	char *rename_target = NULL;
	char *rename_tmpdir = NULL;
	int ret;
	struct stat s;

	tmp = strdup(file->filename);
	tmp2 = strdup(file->filename);

	dir = dirname(tmp);
	base = basename(tmp2);

	rel_dir = dir;
	if (*dir == '/') {
		rel_dir = dir + 1;
	}

	string_or_die(&original, "%s/staged/%s", state_dir, file->hash);

	string_or_die(&targetpath, "%s%s", path_prefix, rel_dir);
	ret = stat(targetpath, &s);

	if ((ret == -1) && (errno == ENOENT)) {
		fprintf(stderr, "Update target directory does not exist: %s. Trying to fix it\n", targetpath);
		verify_fix_path(dir, MoM);
	} else if (!S_ISDIR(s.st_mode)) {
		fprintf(stderr, "Error: Update target exists but is NOT a directory: %s\n", targetpath);
	}

	free(targetpath);
	string_or_die(&target, "%s%s/.update.%s", path_prefix, rel_dir, base);
	ret = swupd_rm(target);
	if (ret < 0 && ret != -ENOENT) {
		fprintf(stderr, "Error: Failed to remove %s\n", target);
	}

	string_or_die(&statfile, "%s%s", path_prefix, file->filename);

	memset(&s, 0, sizeof(struct stat));
	ret = lstat(statfile, &s);
	if (ret == 0) {
		if ((file->is_dir && !S_ISDIR(s.st_mode)) ||
		    (file->is_link && !S_ISLNK(s.st_mode)) ||
		    (file->is_file && !S_ISREG(s.st_mode))) {
			//file type changed, move old out of the way for new
			ret = swupd_rm(statfile);
			if (ret < 0) {
				ret = -ETYPE_CHANGED_FILE_RM;
				goto out;
			}
		}
	}
	free(statfile);

	if (file->is_dir || S_ISDIR(s.st_mode)) {
		/* In the btrfs only scenario there was an implicit
		 * "create_or_update_dir()" via un-tar-ing a directory.tar after
		 * download and the untar happens in the staging subvolume which
		 * then gets promoted to a "real" usable subvolume.  But for
		 * a live rootfs the directory needs copied out of staged
		 * and into the rootfs.  Tar is a way to copy with
		 * attributes and it includes internal logic that does the
		 * right thing to overlay a directory onto something
		 * pre-existing: */
		/* In order to avoid tar transforms with directories, rename
		 * the directory before and after the tar command */
		string_or_die(&rename_tmpdir, "%s/tmprenamedir", state_dir);
		ret = create_staging_renamedir(rename_tmpdir);
		if (ret) {
			goto out;
		}
		string_or_die(&rename_target, "%s/%s", rename_tmpdir, base);
		if (rename(original, rename_target)) {
			ret = -errno;
			goto out;
		}
		string_or_die(&tarcommand, TAR_COMMAND " -C '%s' " TAR_PERM_ATTR_ARGS " -cf - './%s' 2> /dev/null | " TAR_COMMAND " -C '%s%s' " TAR_PERM_ATTR_ARGS " -xf - 2> /dev/null",
			      rename_tmpdir, base, path_prefix, rel_dir);
		ret = system(tarcommand);
		if (WIFEXITED(ret)) {
			ret = WEXITSTATUS(ret);
		}
		free(tarcommand);
		if (rename(rename_target, original)) {
			ret = -errno;
			goto out;
		}
		if (ret < 0) {
			ret = -EDIR_OVERWRITE;
			goto out;
		}
	} else { /* (!file->is_dir && !S_ISDIR(stat.st_mode)) */
		/* can't naively hard link(): Non-read-only files with same hash must remain
		 * separate copies otherwise modifications to one instance of the file
		 * propagate to all instances of the file perhaps causing subtle data corruption from
		 * a user's perspective.  In practice the rootfs is stateless and owned by us.
		 * Additionally cross-mount hardlinks fail and it's hard to know what an admin
		 * might have for overlaid mounts.  The use of tar is a simple way to copy, but
		 * inefficient.  So prefer hardlink and fall back if needed: */
		ret = -1;
		if (!file->is_config && !file->is_state && !file->use_xattrs) {
			ret = link(original, target);
		}
		if (ret < 0) {
			/* either the hardlink failed, or it was undesirable (config), do a tar-tar dance */
			/* In order to avoid tar transforms, rename the file
			 * before and after the tar command */
			string_or_die(&rename_target, "%s/staged/.update.%s", state_dir, base);
			ret = rename(original, rename_target);
			if (ret) {
				ret = -errno;
				goto out;
			}
			string_or_die(&tarcommand, TAR_COMMAND " -C '%s/staged' " TAR_PERM_ATTR_ARGS " -cf - '.update.%s' 2> /dev/null | " TAR_COMMAND " -C '%s%s' " TAR_PERM_ATTR_ARGS " -xf - 2> /dev/null",
				      state_dir, base, path_prefix, rel_dir);
			ret = system(tarcommand);
			if (WIFEXITED(ret)) {
				ret = WEXITSTATUS(ret);
			}
			free(tarcommand);
			ret = rename(rename_target, original);
			if (ret) {
				ret = -errno;
				goto out;
			}
		}

		struct stat buf;
		int err;

		if (file->staging) {
			/* this must never happen...full file never finished download */
			free(file->staging);
			file->staging = NULL;
		}

		string_or_die(&file->staging, "%s%s/.update.%s", path_prefix, rel_dir, base);

		err = lstat(file->staging, &buf);
		if (err != 0) {
			free(file->staging);
			file->staging = NULL;
			ret = -EDOTFILE_WRITE;
			goto out;
		}
	}

out:
	free(target);
	free(original);
	free(rename_target);
	free(rename_tmpdir);
	free(tmp);
	free(tmp2);

	return ret;
}

/* caller should not call this function for do_not_update marked files */
int rename_staged_file_to_final(struct file *file)
{
	int ret;
	char *target;

	string_or_die(&target, "%s%s", path_prefix, file->filename);

	if (!file->staging && !file->is_deleted && !file->is_dir) {
		free(target);
		return -1;
	}

	if (file->is_deleted) {
		ret = swupd_rm(target);

		/* don't count missing ones as errors...
		 * if somebody already deleted them for us then all is well */
		if ((ret == -ENOENT) || (ret == -ENOTDIR)) {
			ret = 0;
		}
	} else if (file->is_dir) {
		ret = 0;
	} else {
		struct stat stat;
		ret = lstat(target, &stat);

		/* If the file was previously a directory but no longer, then
		 * we need to move it out of the way.
		 * This should not happen because the server side complains
		 * when creating update content that includes such a state
		 * change.  But...you never know. */

		if ((ret == 0) && (S_ISDIR(stat.st_mode))) {
			char *lostnfound;
			char *base;

			string_or_die(&lostnfound, "%slost+found", path_prefix);
			ret = mkdir(lostnfound, S_IRWXU);
			if ((ret != 0) && (errno != EEXIST)) {
				free(lostnfound);
				free(target);
				return ret;
			}
			free(lostnfound);

			base = basename(file->filename);
			string_or_die(&lostnfound, "%slost+found/%s", path_prefix, base);
			/* this will fail if the directory was not already emptied */
			ret = rename(target, lostnfound);
			if (ret < 0 && errno != ENOTEMPTY && errno != EEXIST) {
				fprintf(stderr, "Error: failed to move %s to lost+found: %s\n",
					base, strerror(errno));
			}
			free(lostnfound);
		} else {
			ret = rename(file->staging, target);
			if (ret < 0) {
				fprintf(stderr, "Error: failed to rename staged %s to final: %s\n",
					file->hash, strerror(errno));
			}
			unlink(file->staging);
		}
	}

	free(target);
	return ret;
}

int rename_all_files_to_final(struct list *updates)
{
	int ret, update_errs = 0, update_good = 0, skip = 0;
	struct list *list;

	list = list_head(updates);
	while (list) {
		struct file *file;
		file = list->data;
		list = list->next;
		if (file->do_not_update) {
			skip += 1;
			continue;
		}

		ret = rename_staged_file_to_final(file);
		if (ret != 0) {
			update_errs += 1;
		} else {
			update_good += 1;
		}
	}

	return update_count - update_good - update_errs - (update_skip - skip);
}
