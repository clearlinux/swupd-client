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
#include "swupd.h"
#include "xattrs.h"

static char *file_full_path(const struct file *file)
{
	return sys_path_join(path_prefix, file->filename);
}

static char *file_tmp_full_path(const struct file *file)
{
	char *rel_dir, *destination;

	if (file->is_dir) {
		return file_full_path(file);
	}

	rel_dir = sys_dirname(file->filename);
	destination = str_or_die("%s%s/.update.%s", path_prefix, rel_dir,
				 basename(file->filename));
	free(rel_dir);

	return destination;
}

static char *file_staged_path(const struct file *file)
{
	return str_or_die("%s/staged/%s", state_dir, file->hash);
}

/**
 * @brief Stage the file in the system.
 *
 * If it's a directory, copy it to final destination. If it's a file or symlink
 * install it in the final directory, but with a temporary name.
 */
static int stage_file(struct file *file)
{
	char *staged, *destination;
	int ret = 0;

	//TODO: Stop saving the staging path
	free(file->staging);
	file->staging = destination = file_tmp_full_path(file);

	// Remove any file in destination location
	ret = rm_rf(destination);
	if (ret != 0) {
		ret = -1;
		goto exit;
	}

	staged = file_staged_path(file);

	// Copy file to destination
	ret = copy_all(staged, destination);
	free(staged);
exit:
	return ret;
}

static int ensure_basedir_is_installed(const char *file, struct manifest *MoM)
{
	struct file *dir_file;
	char *real_dir, *dir;
	int err = 0;

	dir = sys_dirname(file);
	real_dir = sys_path_join(path_prefix, dir);
	sys_remove_trailing_path_separators(real_dir);

	// If there's any symlink in the path of this file
	if (!sys_is_absolute_path(real_dir)) {
		err = ensure_basedir_is_installed(dir, MoM);
		if (err) {
			goto exit;
		}
	}

	if (!sys_is_directory(real_dir)) {
		err = unlink(real_dir);
		if (err && errno != ENOENT) {
			err = -errno;
			goto exit;
		}

		dir_file = search_file_in_manifest(MoM, dir);
		if (!dir_file) {
			err = -ENOENT;
			goto exit;
		}

		err = download_single_fullfile(dir_file);
		if (err != 0) {
			error("Problem downloading file %s\n", dir_file->filename);
			goto exit;
		}
		err = stage_file(dir_file);
	}

exit:
	free(dir);
	free(real_dir);
	return err;
}

static int apply_dir_attributes(struct file *file)
{
	char *staged = NULL, *destination = NULL;
	struct stat stat = { 0 };
	int err = 0;

	destination = file_tmp_full_path(file);
	staged = file_staged_path(file);

	if (lstat(staged, &stat) < 0) {
		err = -errno;
		goto exit;

	}
	// Change owner
	err = chown(destination, stat.st_uid, stat.st_gid);
	if (err) {
		err = -errno;
		goto exit;
	}

	// Change permissions
	err = chmod(destination, stat.st_mode);
	if (err) {
		err = -errno;
		goto exit;
	}

	// Copy extended permissions
	xattrs_copy(staged, destination);

exit:
	free(staged);
	free(destination);
	return err;
}

enum swupd_code do_staging(struct file *file, struct manifest *MoM)
{
	int ret = 0;
	char *full_path, *full_dir;

	// Make sure target directory exists and it's not a symlink
	ret = ensure_basedir_is_installed(file->filename, MoM);
	if (ret != 0) {
		return ret;
	}

	full_path = file_full_path(file);
	full_dir = sys_dirname(full_path);

	if (!sys_is_absolute_path(full_dir)) {
		error("File %s has not an absolute filename\n", file->filename);
		error("Manifest shouldn't include a file relative path - Aborting stating\n");
		ret = -1;
		goto exit;
	}

	if (file->is_dir && sys_is_directory(full_path)) {
		ret = apply_dir_attributes(file);
	} else {
		ret = stage_file(file);
	}

exit:
	free(full_path);
	free(full_dir);

	return 0;
}

/* caller should not call this function for do_not_update marked files */
int rename_staged_file_to_final(struct file *file)
{
	int ret;
	char *target;

	string_or_die(&target, "%s%s", path_prefix, file->filename);

	if (!file->staging && !file->is_deleted && !file->is_dir) {
		free_string(&target);
		return -1;
	}

	/* Delete files if they are not ghosted and will be garbage collected by
	 * another process */
	if (file->is_deleted && !file->is_ghosted) {
		ret = swupd_rm(target);

		/* don't count missing ones as errors...
		 * if somebody already deleted them for us then all is well */
		if ((ret == -ENOENT) || (ret == -ENOTDIR)) {
			ret = 0;
		}
	} else if (file->is_dir || file->is_ghosted) {
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
				free_string(&lostnfound);
				free_string(&target);
				return ret;
			}
			free_string(&lostnfound);

			base = basename(file->filename);
			string_or_die(&lostnfound, "%slost+found/%s", path_prefix, base);
			/* this will fail if the directory was not already emptied */
			ret = rename(target, lostnfound);
			if (ret < 0 && errno != ENOTEMPTY && errno != EEXIST) {
				error("failed to move %s to lost+found: %s\n",
				      base, strerror(errno));
			}
			free_string(&lostnfound);
		} else {
			ret = rename(file->staging, target);
			if (ret < 0) {
				error("failed to rename staged %s to final: %s\n",
				      file->hash, strerror(errno));
			}
			unlink(file->staging);
		}
	}

	free_string(&target);
	return ret;
}

int rename_all_files_to_final(struct list *updates)
{
	int ret, update_errs = 0, update_good = 0, skip = 0;
	struct list *list;
	unsigned int complete = 0;
	unsigned int list_length = list_len(updates);

	list = list_head(updates);
	while (list) {
		struct file *file;
		file = list->data;
		list = list->next;
		complete++;
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

		progress_report(complete, list_length);
	}

	progress_report(list_length, list_length); /* Force out 100% */
	info("\n");
	return update_count - update_good - update_errs - (update_skip - skip);
}
