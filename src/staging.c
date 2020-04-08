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

static enum swupd_code verify_fix_path(char *targetpath, struct manifest *target_mom);

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
//TODO: "stage_single_file is currently not able to be run in parallel"
/* Consider adding a remove_leftovers() that runs in verify/fix in order to
 * allow this function to mkdtemp create folders for parallel build */
enum swupd_code stage_single_file(struct file *file, struct manifest *mom)
{
	char *statfile = NULL;
	char *dir, *base;
	char *tarcommand = NULL;
	char *original = NULL;
	char *target = NULL;
	char *targetpath = NULL;
	char *rename_target = NULL;
	char *rename_tmpdir = NULL;
	char real_path[4096] = { 0 };
	struct stat s;
	int ret;

	dir = sys_dirname(file->filename);
	base = sys_basename(file->filename);

	original = sys_path_join("%s/staged/%s", globals.state_dir, file->hash);

	/* make sure the directory where the file should be copied to exists
	 * and is in deed a directory */
	targetpath = sys_path_join("%s/%s", globals.path_prefix, dir);
	if (!sys_filelink_exists(targetpath)) {
		if (mom) {
			verify_fix_path(dir, mom);
		} else {
			debug("Target directory does not exist: %s. Auto-fix disabled\n", targetpath);
		}
	} else if (!sys_filelink_is_dir(targetpath)) {
		error("Target exists but is NOT a directory: %s\n", targetpath);
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
	target = sys_path_join("%s/.update.%s", targetpath, base);
	ret = sys_rm_recursive(target);
	if (ret != 0 && ret != -ENOENT) {
		ret = SWUPD_COULDNT_REMOVE_FILE;
		error("Failed to remove %s\n", target);
		goto out;
	}

	/* if the file already exists in the final destination, check to see
	 * if it is of the same type */
	statfile = sys_path_join("%s/%s", globals.path_prefix, file->filename);
	memset(&s, 0, sizeof(struct stat));
	ret = lstat(statfile, &s);
	if (ret == 0) {
		if ((file->is_dir && !S_ISDIR(s.st_mode)) ||
		    (file->is_link && !S_ISLNK(s.st_mode)) ||
		    (file->is_file && !S_ISREG(s.st_mode))) {
			// file type changed, move old out of the way for new
			ret = sys_rm_recursive(statfile);
			free_and_clear_pointer(&statfile);
			if (ret != 0 && ret != -ENOENT) {
				ret = SWUPD_COULDNT_REMOVE_FILE;
				goto out;
			}
		}
	}
	free_and_clear_pointer(&statfile);

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
		rename_tmpdir = sys_path_join("%s/tmprenamedir", globals.state_dir);
		ret = create_staging_renamedir(rename_tmpdir);
		if (ret) {
			ret = SWUPD_COULDNT_CREATE_DIR;
			goto out;
		}
		rename_target = sys_path_join("%s/%s", rename_tmpdir, base);
		if (rename(original, rename_target)) {
			ret = SWUPD_COULDNT_RENAME_DIR;
			goto out;
		}
		string_or_die(&tarcommand, TAR_COMMAND " -C '%s' " TAR_PERM_ATTR_ARGS " -cf - './%s' 2> /dev/null | " TAR_COMMAND " -C '%s' " TAR_PERM_ATTR_ARGS " -xf - 2> /dev/null",
			      rename_tmpdir, base, targetpath);
		ret = system(tarcommand);
		if (ret == -1) {
			ret = SWUPD_SUBPROCESS_ERROR;
		}
		if (WIFEXITED(ret)) {
			ret = WEXITSTATUS(ret);
		}
		free_and_clear_pointer(&tarcommand);
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
		if (!file->is_config && !file->is_state && !file->use_xattrs && !file->is_link) {
			ret = link(original, target);
		}
		if (ret < 0) {
			/* either the hardlink failed, or it was undesirable (config), do a tar-tar dance */
			/* In order to avoid tar transforms, rename the file
			 * before and after the tar command */
			rename_target = sys_path_join("%s/staged/.update.%s", globals.state_dir, base);
			ret = rename(original, rename_target);
			if (ret) {
				ret = SWUPD_COULDNT_RENAME_FILE;
				goto out;
			}
			string_or_die(&tarcommand, TAR_COMMAND " -C '%s/staged' " TAR_PERM_ATTR_ARGS " -cf - '.update.%s' 2> /dev/null | " TAR_COMMAND " -C '%s' " TAR_PERM_ATTR_ARGS " -xf - 2> /dev/null",
				      globals.state_dir, base, targetpath);
			ret = system(tarcommand);
			if (ret == -1) {
				ret = SWUPD_SUBPROCESS_ERROR;
			}
			if (WIFEXITED(ret)) {
				ret = WEXITSTATUS(ret);
			}
			free_and_clear_pointer(&tarcommand);
			ret = rename(rename_target, original);
			if (ret) {
				ret = SWUPD_COULDNT_RENAME_FILE;
				goto out;
			}
		}

		free_and_clear_pointer(&file->staging);
		file->staging = sys_path_join("%s/.update.%s", targetpath, base);
		if (!sys_file_exists(file->staging)) {
			free_and_clear_pointer(&file->staging);
			ret = SWUPD_COULDNT_CREATE_FILE;
			goto out;
		}
	}

out:
	free_and_clear_pointer(&dir);
	free_and_clear_pointer(&target);
	free_and_clear_pointer(&targetpath);
	free_and_clear_pointer(&original);
	free_and_clear_pointer(&rename_target);
	free_and_clear_pointer(&rename_tmpdir);

	return ret;
}

/* This function is meant to be called while staging file to fix any missing/incorrect paths.
 * While staging a file, if its parent directory is missing, this would try to create the path
 * by breaking it into sub-paths and fixing them top down.
 * Here, target_mom is the consolidated manifest for the version you are trying to update/verify.
 */
static enum swupd_code verify_fix_path(char *targetpath, struct manifest *target_mom)
{
	struct list *path_list = NULL; /* path_list contains the subparts in the path */
	char *path;
	char *tmp = NULL, *target = NULL;
	char *url = NULL;
	struct stat sb;
	int ret = SWUPD_OK;
	struct file *file;
	char *tar_dotfile = NULL;
	struct list *list1 = NULL;

	/* This shouldn't happen */
	if (strcmp(targetpath, "/") == 0) {
		return ret;
	}

	/* Removing trailing '/' from the path */
	path = strdup_or_die(targetpath);
	if (path[string_len(path) - 1] == '/') {
		path[string_len(path) - 1] = '\0';
	}

	/* Breaking down the path into parts.
	 * eg. Path /usr/bin/foo will be broken into /usr,/usr/bin and /usr/bin/foo
	 */
	while (strcmp(path, "/") != 0) {
		path_list = list_prepend_data(path_list, strdup_or_die(path));
		tmp = sys_dirname(path);
		free_and_clear_pointer(&path);
		path = tmp;
	}
	free_and_clear_pointer(&path);

	list1 = list_head(path_list);
	while (list1) {
		path = list1->data;
		list1 = list1->next;

		free_and_clear_pointer(&target);
		free_and_clear_pointer(&tar_dotfile);
		free_and_clear_pointer(&url);

		target = sys_path_join("%s/%s", globals.path_prefix, path);

		/* Search for the file in the manifest, to get the hash for the file */
		file = search_file_in_manifest(target_mom, path);
		if (file == NULL) {
			error("Path %s not found in any of the subscribed manifests"
			      "in verify_fix_path for path_prefix %s\n",
			      path, globals.path_prefix);
			ret = SWUPD_PATH_NOT_IN_MANIFEST;
			goto end;
		}

		if (file->is_deleted) {
			error("Path %s found deleted in verify_fix_path\n", path);
			ret = SWUPD_UNEXPECTED_CONDITION;
			goto end;
		}

		ret = stat(target, &sb);
		if (ret == 0) {
			if (verify_file(file, target)) {
				/* this subpart of the path does exist, nothing to be done */
				continue;
			}
			warn_unlabeled(" -> Corrupt directory: %s", target);
		} else if (ret == -1 && errno == ENOENT) {
			warn_unlabeled(" -> Missing directory: %s", target);
		} else {
			goto end;
		}

		/* In some circumstances (Docker using layers between updates/bundle adds,
		 * corrupt staging content) we could have content which fails to stage.
		 * In order to avoid this causing failure in verify_fix_path, remove the
		 * staging content before proceeding. This also cleans up in case any prior
		 * download failed in a partial state.
		 */
		unlink_all_staged_content(file);

		/* download the fullfile for the missing path */
		string_or_die(&tar_dotfile, "%s/download/.%s.tar", globals.state_dir, file->hash);
		string_or_die(&url, "%s/%i/files/%s.tar", globals.content_url, file->last_change, file->hash);
		ret = swupd_curl_get_file(url, tar_dotfile);
		if (ret != 0) {
			warn_unlabeled(" -> not fixed\n");
			error("Failed to download file %s in verify_fix_path\n", file->filename);
			ret = SWUPD_COULDNT_DOWNLOAD_FILE;
			unlink(tar_dotfile);
			goto end;
		}

		if (untar_full_download(file) != 0) {
			warn_unlabeled(" -> not fixed\n");
			error("Failed to untar file %s\n", file->filename);
			ret = SWUPD_COULDNT_UNTAR_FILE;
			goto end;
		}

		ret = stage_single_file(file, target_mom);
		if (ret != 0) {
			/* stage_single_file returns a swupd_code on error,
			* just propagate the error */
			warn_unlabeled(" -> not fixed\n");
			error("Path %s failed to stage in verify_fix_path\n", path);
			goto end;
		}
		warn_unlabeled(" -> fixed\n");
	}
end:
	free_and_clear_pointer(&target);
	free_and_clear_pointer(&tar_dotfile);
	free_and_clear_pointer(&url);
	list_free_list_and_data(path_list, free);
	return ret;
}

/* caller should not call this function for do_not_update marked files */
int rename_staged_file_to_final(struct file *file)
{
	int ret = 0;
	char *target;
	char *target_path = NULL;

	target = sys_path_join("%s/%s", globals.path_prefix, file->filename);

	if (!file->staging && !file->is_deleted && !file->is_dir) {
		free_and_clear_pointer(&target);
		return -1;
	}

	/* Delete files if they are not ghosted and will be garbage collected by
	 * another process */
	if (file->is_deleted && !file->is_ghosted) {
		/* only delete the file if we can reach it without following symlinks
		 * or we might end up deleting something else */
		target_path = sys_dirname(target);
		if (sys_path_is_absolute(target_path)) {
			ret = sys_rm_recursive(target);

			/* don't count missing ones as errors...
			 * if somebody already deleted them for us then all is well */
			if ((ret == -ENOENT) || (ret == -ENOTDIR)) {
				ret = 0;
			}
		}
		free_and_clear_pointer(&target_path);
	} else if (file->is_dir || file->is_ghosted) {
		ret = 0;
	} else {
		/* If the file was previously a directory but no longer, then
		 * we need to move it out of the way.
		 * This should not happen because the server side complains
		 * when creating update content that includes such a state
		 * change.  But...you never know. */

		if (sys_is_dir(target)) {
			char *lostnfound;
			char *base;

			lostnfound = sys_path_join("%s/lost+found", globals.path_prefix);
			ret = mkdir(lostnfound, S_IRWXU);
			if ((ret != 0) && (errno != EEXIST)) {
				free_and_clear_pointer(&lostnfound);
				free_and_clear_pointer(&target);
				return ret;
			}
			free_and_clear_pointer(&lostnfound);

			base = basename(file->filename);
			lostnfound = sys_path_join("%s/lost+found/%s", globals.path_prefix, base);
			/* this will fail if the directory was not already emptied */
			ret = rename(target, lostnfound);
			if (ret < 0 && errno != ENOTEMPTY && errno != EEXIST) {
				error("failed to move %s to lost+found: %s\n",
				      base, strerror(errno));
			}
			free_and_clear_pointer(&lostnfound);
		} else {
			ret = rename(file->staging, target);
			if (ret < 0) {
				error("failed to rename staged %s to final: %s\n",
				      file->hash, strerror(errno));
			}
			unlink(file->staging);
		}
	}

	free_and_clear_pointer(&target);
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

static enum swupd_code stage_files(struct list *files, struct manifest *mom)
{
	struct list *iter;
	int list_length = list_len(files);
	struct file *file;
	int ret = SWUPD_OK;
	int complete = 0;

	iter = list_head(files);
	while (iter) {
		file = iter->data;
		iter = iter->next;

		progress_report(complete++, list_length * 2);
		if (file->do_not_update || file->is_deleted) {
			continue;
		}

		/*
		 * todo: hash check - We check all hashes before calling this
		 * function, but it would be good to check here if the hash
		 * was really validated for every file to provent future bugs. */
		ret = stage_single_file(file, mom);
		if (ret != SWUPD_OK) {
			error("File staging failed: %s\n", file->filename);
			return ret;
		}
	}
	progress_report(complete, list_length * 2);

	return SWUPD_OK;
}


enum swupd_code staging_install_single_file(struct file *file, struct manifest *mom)
{
	enum swupd_code ret;
	int err;

	ret = stage_single_file(file, mom);
	if (ret != SWUPD_OK) {
		return ret;
	}

	err = rename_staged_file_to_final(file);
	if (err) {
		return SWUPD_COULDNT_RENAME_FILE;
	}

	return SWUPD_OK;
}

enum swupd_code staging_install_files(struct list *files, struct manifest *mom)
{
	int ret = SWUPD_OK;

	if (!list_is_sorted(files, cmp_file_filename_is_deleted)) {
		debug("List of files to install is not sorted - fixing\n");
		files = list_sort(files, cmp_file_filename_is_deleted);
	}

	/*********** rootfs critical section starts ***************************
	 * NOTE: the next loop calls stage_single_file() which can remove files,
	 * starting a critical section.
	 * from here onward we're doing real update work modifying "the disk" */

	info("Installing files...\n");

	/* Install directories and stage regular files */
	ret = stage_files(files, mom);
	if (ret != SWUPD_OK) {
		ret = SWUPD_COULDNT_RENAME_FILE;
		return ret;
	}

	/* sync */
	sync();

	/* rename to apply update */
	ret = rename_all_files_to_final(files);
	if (ret != SWUPD_OK) {
		ret = SWUPD_COULDNT_RENAME_FILE;
		return ret;
	}

	/* sync */
	sync();

	/*********** critical section ends ************************************/

	return SWUPD_OK;
}

/* Iterate the file list and remove from the file system each file/directory */
int staging_remove_files(struct list *files)
{
	struct list *iter = NULL;
	struct file *file = NULL;
	char *fullfile = NULL;
	int total = list_len(files);
	int deleted = total;
	int count = 0;

	iter = list_head(files);
	while (iter) {
		file = iter->data;
		iter = iter->next;
		string_or_die(&fullfile, "%s/%s", globals.path_prefix, file->filename);
		if (sys_rm_recursive(fullfile) == -1) {
			/* if a -1 is returned it means there was an issue deleting the
			 * file or directory, in that case decrease the counter of deleted
			 * files.
			 * Note: If a file didn't exist it will still be counted as deleted,
			 * this is a limitation */
			deleted--;
		}
		free_and_clear_pointer(&fullfile);
		count++;
		progress_report(count, total);
	}

	return deleted;
}
