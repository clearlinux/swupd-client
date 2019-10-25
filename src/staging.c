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
#include "swupd_build_variant.h"

/* clean then recreate temporary folder for tar renames */
static int create_staging_renamedir(char *rename_tmpdir)
{
	int ret;

	if (rm_rf(rename_tmpdir) != 0) {
		/* Not fatal but pretty scary, likely to really fail at the
		 * next command too. Pass for now as printing may just cause
		 * confusion */
		;
	}

	ret = mkdir(rename_tmpdir, S_IRWXU);
	if (ret == -1 && errno != EEXIST) {
		ret = -errno;
	} else {
		ret = 0;
	}

	return ret;
}

/* Do the staging of new files into the filesystem */
//TODO: "do_staging is currently not able to be run in parallel"
/* Consider adding a remove_leftovers() that runs in verify/fix in order to
 * allow this function to mkdtemp create folders for parallel build */
enum swupd_code do_staging(struct file *file, struct manifest *MoM)
{
	char *statfile = NULL, *tmp = NULL, *tmp2 = NULL;
	char *dir, *base, *rel_dir;
	char *tarcommand = NULL;
	char *original = NULL;
	char *target = NULL;
	char *targetpath = NULL;
	char *rename_target = NULL;
	char *rename_tmpdir = NULL;
	char real_path[4096] = { 0 };
	struct stat s;
	struct stat buf;
	int err;
	int ret;

	tmp = strdup_or_die(file->filename);
	tmp2 = strdup_or_die(file->filename);

	dir = dirname(tmp);
	base = basename(tmp2);

	rel_dir = dir;
	if (*dir == '/') {
		rel_dir = dir + 1;
	}

	string_or_die(&original, "%s/staged/%s", globals.state_dir, file->hash);

	/* make sure the directory where the file should be copied to exists
	 * and is in deed a directory */
	string_or_die(&targetpath, "%s%s", globals.path_prefix, rel_dir);
	ret = stat(targetpath, &s);
	if ((ret == -1) && (errno == ENOENT)) {
		if (MoM) {
			warn("Update target directory does not exist: %s. Trying to fix it\n", targetpath);
			verify_fix_path(dir, MoM);
		} else {
			debug("Update target directory does not exist: %s. Auto-fix disabled\n", targetpath);
		}
	} else if (!S_ISDIR(s.st_mode)) {
		error("Update target exists but is NOT a directory: %s\n", targetpath);
	}

	if (!realpath(targetpath, real_path)) {
		/* if the target directory didn't exist and it failed to be fixed
		 * it will end up here */
		ret = SWUPD_COULDNT_CREATE_DIR;
		goto out;
	} else if (strcmp(globals.path_prefix, targetpath) != 0 &&
		   strcmp(targetpath, real_path) != 0) {
		/*
		 * targetpath and real_path should always be equal but
		 * in the case of the targetpath being the path_prefix
		 * there is a trailing '/' in path_prefix but realpath
		 * doesn't keep the trailing '/' so check for that case
		 * specifically.
		 */
		ret = SWUPD_UNEXPECTED_CONDITION;
		goto out;
	}

	/* remove a pre-existing .update file in the destination if it exists */
	string_or_die(&target, "%s%s/.update.%s", globals.path_prefix, rel_dir, base);
	ret = sys_rm_recursive(target);
	if (ret != 0 && ret != -ENOENT) {
		ret = SWUPD_COULDNT_REMOVE_FILE;
		error("Failed to remove %s\n", target);
		goto out;
	}

	/* if the file already exists in the final destination, check to see
	 * if it is of the same type */
	string_or_die(&statfile, "%s%s", globals.path_prefix, file->filename);
	memset(&s, 0, sizeof(struct stat));
	ret = lstat(statfile, &s);
	if (ret == 0) {
		if ((file->is_dir && !S_ISDIR(s.st_mode)) ||
		    (file->is_link && !S_ISLNK(s.st_mode)) ||
		    (file->is_file && !S_ISREG(s.st_mode))) {
			// file type changed, move old out of the way for new
			ret = sys_rm_recursive(statfile);
			if (ret != 0 && ret != -ENOENT) {
				ret = SWUPD_COULDNT_REMOVE_FILE;
				goto out;
			}
		}
	}
	free_string(&statfile);

	/* copy the file/directory to its final destination, if it is a file
	 * keep its name with a .update prefix for now like this .update.(file_name)
	 * if it is a directory it will be renamed to its final name once copied */
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
		string_or_die(&rename_tmpdir, "%s/tmprenamedir", globals.state_dir);
		ret = create_staging_renamedir(rename_tmpdir);
		if (ret) {
			ret = SWUPD_COULDNT_CREATE_DIR;
			goto out;
		}
		string_or_die(&rename_target, "%s/%s", rename_tmpdir, base);
		if (rename(original, rename_target)) {
			ret = SWUPD_COULDNT_RENAME_DIR;
			goto out;
		}
		string_or_die(&tarcommand, TAR_COMMAND " -C '%s' " TAR_PERM_ATTR_ARGS " -cf - './%s' 2> /dev/null | " TAR_COMMAND " -C '%s%s' " TAR_PERM_ATTR_ARGS " -xf - 2> /dev/null",
			      rename_tmpdir, base, globals.path_prefix, rel_dir);
		ret = system(tarcommand);
		if (ret == -1) {
			ret = SWUPD_SUBPROCESS_ERROR;
		}
		if (WIFEXITED(ret)) {
			ret = WEXITSTATUS(ret);
		}
		free_string(&tarcommand);
		if (rename(rename_target, original)) {
			ret = SWUPD_COULDNT_RENAME_DIR;
			goto out;
		}
		if (ret) {
			ret = SWUPD_COULDNT_RENAME_DIR;
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
			string_or_die(&rename_target, "%s/staged/.update.%s", globals.state_dir, base);
			ret = rename(original, rename_target);
			if (ret) {
				ret = SWUPD_COULDNT_RENAME_FILE;
				goto out;
			}
			string_or_die(&tarcommand, TAR_COMMAND " -C '%s/staged' " TAR_PERM_ATTR_ARGS " -cf - '.update.%s' 2> /dev/null | " TAR_COMMAND " -C '%s%s' " TAR_PERM_ATTR_ARGS " -xf - 2> /dev/null",
				      globals.state_dir, base, globals.path_prefix, rel_dir);
			ret = system(tarcommand);
			if (ret == -1) {
				ret = SWUPD_SUBPROCESS_ERROR;
			}
			if (WIFEXITED(ret)) {
				ret = WEXITSTATUS(ret);
			}
			free_string(&tarcommand);
			ret = rename(rename_target, original);
			if (ret) {
				ret = SWUPD_COULDNT_RENAME_FILE;
				goto out;
			}
		}

		free_string(&file->staging);
		string_or_die(&file->staging, "%s%s/.update.%s", globals.path_prefix, rel_dir, base);
		err = lstat(file->staging, &buf);
		if (err != 0) {
			free_string(&file->staging);
			ret = SWUPD_COULDNT_CREATE_FILE;
			goto out;
		}
	}

out:
	free_string(&target);
	free_string(&targetpath);
	free_string(&original);
	free_string(&rename_target);
	free_string(&rename_tmpdir);
	free_string(&tmp);
	free_string(&tmp2);

	return ret;
}

/* caller should not call this function for do_not_update marked files */
int rename_staged_file_to_final(struct file *file)
{
	int ret;
	char *target;

	string_or_die(&target, "%s%s", globals.path_prefix, file->filename);

	if (!file->staging && !file->is_deleted && !file->is_dir) {
		free_string(&target);
		return -1;
	}

	/* Delete files if they are not ghosted and will be garbage collected by
	 * another process */
	if (file->is_deleted && !file->is_ghosted) {
		ret = sys_rm_recursive(target);

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

			string_or_die(&lostnfound, "%slost+found", globals.path_prefix);
			ret = mkdir(lostnfound, S_IRWXU);
			if ((ret != 0) && (errno != EEXIST)) {
				free_string(&lostnfound);
				free_string(&target);
				return ret;
			}
			free_string(&lostnfound);

			base = basename(file->filename);
			string_or_die(&lostnfound, "%slost+found/%s", globals.path_prefix, base);
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

static int rename_all_files_to_final(struct list *updates)
{
	int ret, update_errs = 0, update_good = 0, skip = 0;
	struct list *list;
	int complete = 0;
	int list_length = list_len(updates);

	list = list_head(updates);
	while (list) {
		struct file *file;
		file = list->data;
		list = list->next;

		complete++;
		if (file->do_not_update) {
			skip += 1;
			goto progress;
			;
		}

		ret = rename_staged_file_to_final(file);
		if (ret != 0) {
			update_errs += 1;
		} else {
			update_good += 1;
		}

	progress:
		progress_report(list_length + complete, list_length * 2);
	}

	return globals.update_count - update_good - update_errs - (globals.update_skip - skip);
}

enum swupd_code staging_install_all_files(struct list *files, struct manifest *mom)
{
	struct list *iter;
	struct file *file;
	int ret = 0;
	int complete = 0;
	unsigned int list_length = list_len(files);

	/*********** rootfs critical section starts ***************************
NOTE: the next loop calls do_staging() which can remove files, starting a critical section
which ends after rename_all_files_to_final() succeeds
	 */

	/* from here onward we're doing real update work modifying "the disk" */

	/* starting at list_head in the filename alpha-sorted updates list
	 * means node directories are added before leaf files */
	info("Installing files...\n");
	iter = list_head(files);
	while (iter) {
		file = iter->data;
		iter = iter->next;

		progress_report(complete++, list_length * 2);
		if (file->do_not_update || file->is_deleted) {
			continue;
		}

		/* for each file: fdatasync to persist changed content over reboot, or maybe a global sync */
		/* for each file: check hash value; on mismatch delete and queue full download */
		/* todo: hash check */

		ret = do_staging(file, mom);
		if (ret != 0) {
			error("File staging failed: %s\n", file->filename);
			return ret;
		}
	}
	progress_report(complete, list_length * 2);

	/* check policy, and if policy says, "ask", ask the user at this point */
	/* check for reboot need - if needed, wait for reboot */

	/* sync */
	sync();

	/* rename to apply update */
	ret = rename_all_files_to_final(files);
	if (ret != 0) {
		ret = SWUPD_COULDNT_RENAME_FILE;
		return ret;
	}

	/* TODO: do we need to optimize directory-permission-only changes (directories
	 *       are now sent as tar's so permissions are handled correctly, even
	 *       if less than efficiently)? */

	sync();

	/* NOTE: critical section starts when update_loop() calls do_staging() */
	/*********** critical section ends *************************************/

	return 0;
}
