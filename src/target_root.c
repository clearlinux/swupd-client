/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2020 Intel Corporation.
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

#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "swupd.h"
#include "swupd_build_variant.h"
#include "xattrs.h"

#define STAGE_FILE_PREFIX ".update."

static bool verify_fix_path(char *dir, struct manifest *mom);

/* clean then recreate temporary folder for tar renames */
static int create_staging_renamedir(char *rename_tmpdir)
{
	int ret;

	if (rm_rf(rename_tmpdir) != 0) {
		/* Not fatal but pretty scary, likely to really fail at the
		 * next command too. Pass for now as printing may just cause
		 * confusion */
		UNEXPECTED();
	}

	ret = mkdir(rename_tmpdir, S_IRWXU);
	if (ret == -1 && errno != EEXIST) {
		ret = -errno;
	} else {
		ret = 0;
	}

	return ret;
}

static bool file_type_changed(const char *statfile, struct file *file)
{
	struct stat s = { 0 };
	int ret;

	ret = lstat(statfile, &s);

	// File exists, but stat failed. Consider a file type change
	// to ensure file is removed before updating
	if (ret != 0 && errno != ENOENT && errno != ENOTDIR) {
		UNEXPECTED();
		return true;
	}

	return (ret == 0) &&				  // File exists
	       ((file->is_dir && !S_ISDIR(s.st_mode)) ||  // Changed to dir
		(file->is_link && !S_ISLNK(s.st_mode)) || // Changed to link
		(file->is_file && !S_ISREG(s.st_mode)));  // Changed to file
}

static int run_tar_cmd_params(bool extract, const char *dir, const char *archive, const char *basename)
{
#ifdef TAR_XATTR_ARGS
#define PARAMS_SIZE 20
#else
#define PARAMS_SIZE 8
#endif
	const char *params[PARAMS_SIZE] = { 0 };
	int i = 0;

	params[i++] = TAR_COMMAND;
	params[i++] = "-C";
	params[i++] = dir;
	params[i++] = extract ? "-xf" : "-cf";
	params[i++] = archive;
	if (basename) {
		params[i++] = basename;
	}
	params[i++] = TAR_PERM_ATTR_ARGS;

#ifdef TAR_XATTR_ARGS
	char *str = strdup_or_die(TAR_XATTR_ARGS);
	char *tok;
	char *ctx = NULL;

	for (tok = strtok_r(str, " ", &ctx); tok; tok = strtok_r(NULL, " ", &ctx)) {
		// If params array is not full
		if (i < PARAMS_SIZE - 1) {
			params[i++] = tok;
		} else {
			debug("skipping tar parameter %s - too many parameters\n", tok);
		}
	}
	FREE(str);
#endif

	params[i] = NULL;
	return run_command_params_quiet((char **)params);
#undef PARAMS_SIZE
}

static int tartar(const char *source_dir, const char *source_basename, const char *target_dir)
{
	static const bool extract = true;
	static const bool create = false;

	char *tmp_archive;
	int err = 0, rm_err;

	tmp_archive = sys_path_join("%s/tmparchive.tar", globals.state_dir);

	rm_err = sys_rm_recursive(tmp_archive);
	if (rm_err && rm_err != -ENOENT) {
		debug("Failed to remove archive file %s %d", tmp_archive, rm_err);
		UNEXPECTED();
		// Removing archive fail. Trying anyway, because if it has the
		// same type it will work
	}

	// Create tarball
	err = run_tar_cmd_params(create, source_dir, tmp_archive, source_basename);
	if (err) {
		debug("Failed to create tarball to copy file %s %d",
		      tmp_archive, err);
		goto cleanup;
	}

	// Extract tarball
	err = run_tar_cmd_params(extract, target_dir, tmp_archive, NULL);
	if (err) {
		debug("Failed to extract tarball to copy file %s %d",
		      tmp_archive, err);
	}

cleanup:

	rm_err = sys_rm_recursive(tmp_archive);
	if (rm_err && rm_err != -ENOENT) {
		debug("Failed to remove archive file %s %d", tmp_archive, rm_err);
		UNEXPECTED();
	}

	FREE(tmp_archive);
	return err;
}

static enum swupd_code install_dir_using_tar(const char *fullfile_path, const char *target_file)
{
	int err;
	enum swupd_code ret = SWUPD_OK;
	char *rename_tmpdir = NULL;
	char *rename_target = NULL;
	char *local_basename = NULL;
	char *target_basename = NULL;
	char *target_path = NULL;

	target_basename = sys_basename(target_file);
	local_basename = sys_path_join("./%s", target_basename);
	target_path = sys_dirname(target_file);
	rename_tmpdir = sys_path_join("%s/tmprenamedir", globals.state_dir);
	rename_target = sys_path_join("%s/%s", rename_tmpdir, target_basename);

	/* In order to avoid tar transforms with directories, rename
	 * the directory before and after the tar command */
	if (create_staging_renamedir(rename_tmpdir) != 0) {
		ret = SWUPD_COULDNT_CREATE_DIR;
		goto out;
	}

	if (rename(fullfile_path, rename_target) != 0) {
		ret = SWUPD_COULDNT_RENAME_DIR;
		goto out;
	}

	err = tartar(rename_tmpdir, local_basename, target_path);
	if (err) {
		ret = SWUPD_SUBPROCESS_ERROR;
	}

	if (rename(rename_target, fullfile_path) != 0) {
		ret = SWUPD_COULDNT_RENAME_DIR;
	}

out:
	FREE(rename_tmpdir);
	FREE(rename_target);
	FREE(local_basename);
	FREE(target_path);
	return ret;
}

static enum swupd_code install_file_using_tar(const char *fullfile_path, const char *target_file)
{
	int err;
	enum swupd_code ret = SWUPD_OK;
	char *rename_tmpdir = NULL;
	char *rename_target = NULL;
	char *stage_dir = NULL;
	char *staged_file = NULL;
	char *target_basename = NULL;
	char *target_path = NULL;

	target_basename = sys_basename(target_file);
	target_path = sys_dirname(target_file);

	rename_target = sys_path_join("%s/staged/%s%s", globals.state_dir, STAGE_FILE_PREFIX, target_basename);
	if (rename(fullfile_path, rename_target) != 0) {
		ret = SWUPD_COULDNT_RENAME_FILE;
		goto out;
	}

	stage_dir = sys_path_join("%s/staged", globals.state_dir);
	staged_file = str_or_die("%s%s", STAGE_FILE_PREFIX, target_basename);
	err = tartar(stage_dir, staged_file, target_path);
	if (err) {
		ret = SWUPD_SUBPROCESS_ERROR;
	}

	if (rename(rename_target, fullfile_path) != 0) {
		ret = SWUPD_COULDNT_RENAME_FILE;
	}

out:
	FREE(rename_tmpdir);
	FREE(rename_target);
	FREE(staged_file);
	FREE(stage_dir);
	FREE(target_path);
	return ret;
}

static enum swupd_code install_staged_file(struct file *file, const char *staged_file, const char *fullfile_path, const char *target_file)
{
#ifndef FORCE_TARTAR
	/* can't naively hard link(): Non-read-only files with same hash must remain
	 * separate copies otherwise modifications to one instance of the file
	 * propagate to all instances of the file perhaps causing subtle data corruption from
	 * a user's perspective.  In practice the rootfs is stateless and owned by us.
	 * Additionally cross-mount hardlinks fail and it's hard to know what an admin
	 * might have for overlaid mounts.  The use of tar is a simple way to copy, but
	 * inefficient.  So prefer hardlink and fall back if needed: */

	if (!file->is_config && !file->is_state && !file->use_xattrs && !file->is_link) {
		if (link(fullfile_path, staged_file) == 0) {
			return SWUPD_OK;
		}
	}

	if (copy(fullfile_path, staged_file) == 0) {
		return SWUPD_OK;
	}

	// If file failed to link or copy
	UNEXPECTED();
#else
	(void)file;
	(void)staged_file;
#endif
	return install_file_using_tar(fullfile_path, target_file);
}

#ifndef FORCE_TARTAR
static int install_dir_using_systools(const char *fullfile_path, const char *target_file)
{
	struct stat s;
	int err;

	err = lstat(fullfile_path, &s);
	if (err < 0) {
		return -errno;
	}

	err = mkdir(target_file, S_IRWXU);
	if (err && errno != EEXIST) {
		// We guarantee that if a file exists it is a directory
		return -errno;
	}

	err = chmod(target_file, s.st_mode);
	if (err) {
		return -errno;
	}

	// Change owner and group
	err = lchown(target_file, s.st_uid, s.st_gid);
	if (err) {
		return -errno;
	}

	// Copy xattrs
	xattrs_copy(fullfile_path, target_file);

	return 0;
}
#endif

static enum swupd_code install_dir(const char *fullfile_path, const char *target_file)
{
#ifndef FORCE_TARTAR
	if (install_dir_using_systools(fullfile_path, target_file) == 0) {
		return SWUPD_OK;
	}

	UNEXPECTED();
#endif
	return install_dir_using_tar(fullfile_path, target_file);
}

/* Do the staging of new files into the filesystem */
//TODO: "stage_single_file is currently not able to be run in parallel"
/* Consider adding a remove_leftovers() that runs in verify/fix in order to
 * allow this function to mkdtemp create folders for parallel build */
static enum swupd_code stage_single_file(struct file *file, struct manifest *mom)
{
	char *target_file = NULL;
	char *dir, *target_basename;
	char *fullfile_path = NULL;
	char *staged_file = NULL;
	char *target_path = NULL;
	int ret;

	dir = sys_dirname(file->filename);
	target_basename = sys_basename(file->filename);

	fullfile_path = sys_path_join("%s/staged/%s", globals.state_dir, file->hash);
	target_path = sys_path_join("%s/%s", globals.path_prefix, dir);
	target_file = sys_path_join("%s/%s", globals.path_prefix, file->filename);
	staged_file = sys_path_join("%s/%s%s", target_path, STAGE_FILE_PREFIX, target_basename);

	/* make sure the directory where the file should be copied to exists
	 * and is indeed a directory */
	if (!sys_filelink_is_dir(target_path)) {
		debug("Target directory is incorect: %s\n", target_path);
		if (mom) {
			debug("Attempting to fix missing directory\n");
			if (!verify_fix_path(dir, mom)) {
				ret = SWUPD_COULDNT_CREATE_DIR;
				goto out;
			}
		} else {
			debug("Auto-fix of incorrect directory disabled\n");
			ret = SWUPD_COULDNT_CREATE_DIR;
			goto out;
		}
	}

	if (!sys_path_is_absolute(target_path)) {
		/* This should never happen with Manifest generated by
		 * mixer, but we are trying to make sure we are not going
		 * to install any file that is pointed by a symlink */
		ret = SWUPD_UNEXPECTED_CONDITION;
		goto out;
	}

	/* remove a pre-existing .update file in the destination if it exists */
	ret = sys_rm_recursive(staged_file);
	if (ret != 0 && ret != -ENOENT) {
		ret = SWUPD_COULDNT_REMOVE_FILE;
		error("Failed to remove staging file %s\n", staged_file);
		goto out;
	}

	/* if the file already exists in the final destination, check to see
	 * if it is of the same type */
	if (file_type_changed(target_file, file)) {
		// file type changed, move old out of the way for new
		debug("The file type changed for file %s, removing old file\n", target_file);
		ret = sys_rm_recursive(target_file);
		if (ret != 0 && ret != -ENOENT) {
			error("Target has different file type but could not be removed: %s\n", target_file);
			ret = SWUPD_COULDNT_REMOVE_FILE;
			goto out;
		}
	}

	/* copy the file/directory to its final destination, if it is a file
	 * keep its name with a .update prefix for now like this .update.(file_name)
	 * if it is a directory it will be renamed to its final name once copied */
	if (file->is_dir) {
		ret = install_dir(fullfile_path, target_file);
	} else { /* (!file->is_dir) */
		ret = install_staged_file(file, staged_file, fullfile_path, target_file);

		if (ret == SWUPD_OK) {
			// Update staging reference in file structure
			FREE(file->staging);
			file->staging = strdup_or_die(staged_file);
		}
	}

out:
	FREE(dir);
	FREE(target_file);
	FREE(staged_file);
	FREE(target_path);
	FREE(fullfile_path);

	return ret;
}

static bool verify_fix_single_directory(const char *path, struct manifest *mom)
{
	bool ret = false;
	struct file *file = NULL;
	char *target_path = NULL;
	char *tar_dotfile = NULL;
	char *url = NULL;

	/* Search for the file in the manifest, to get the hash for the file */
	file = search_file_in_manifest(mom, path);
	if (file == NULL) {
		error("Path %s not found in any of the subscribed manifests\n",
		      path);
		goto end;
	}

	if (file->is_deleted) {
		error("Path %s marked as deleted in manifest\n", path);
		goto end;
	}

	target_path = sys_path_join("%s/%s", globals.path_prefix, path);
	if (sys_file_exists(target_path)) {
		if (verify_file(file, target_path)) {
			/* this subpart of the path does exist, nothing to be done */
			ret = true;
			goto end;
		}
		debug("Corrupt directory: %s\n", target_path);
	} else {
		debug("Missing directory: %s\n", target_path);
	}

	/* In some circumstances, like Docker using layers between updates/bundle adds,
	 * corrupt staging content, we could have corrupted staged content.
	 */
	unlink_all_staged_content(file);

	/* download the fullfile for the missing path */
	tar_dotfile = sys_path_join("%s/download/.%s.tar", globals.state_dir, file->hash);
	url = str_or_die("%s/%i/files/%s.tar", globals.content_url, file->last_change, file->hash);
	ret = swupd_curl_get_file(url, tar_dotfile);
	if (ret != 0) {
		error("Failed to download file %s\n", file->filename);
		unlink(tar_dotfile);
		goto end;
	}

	if (untar_full_download(file) != 0) {
		error("Failed to untar file %s\n", file->filename);
		goto end;
	}

	if (stage_single_file(file, mom) == SWUPD_OK) {
		ret = true;
	} else {
		debug("Path %s failed to stage in verify_fix_path\n", path);
	}

end:
	FREE(url);
	FREE(target_path);
	FREE(tar_dotfile);
	return ret;
}

static struct list *split_directories(const char *dir)
{
	struct list *list_dir = NULL;
	char *path;

	/* Removing trailing '/' from the path */
	path = sys_path_join("%s", dir);
	while (str_cmp(path, "/") != 0) {
		list_dir = list_prepend_data(list_dir, path);
		path = sys_dirname(path);
	}

	FREE(path);
	return list_dir;
}

/* This function is meant to be called while staging file to fix any missing/incorrect paths.
 * While staging a file, if its parent directory is missing, this would try to create the path
 * by breaking it into sub-paths and fixing them top down.
 * Here, mom is the consolidated manifest for the version you are trying to update/verify.
 */
static bool verify_fix_path(char *dir, struct manifest *mom)
{
	bool ret = true;
	char *cur_dir;
	struct list *list_dir, *i;

	list_dir = split_directories(dir);

	/* Loop parents */
	for (i = list_dir; i; i = i->next) {
		cur_dir = i->data;

		// Abort on first error
		if (!verify_fix_single_directory(cur_dir, mom)) {
			ret = false;
			break;
		}
	}

	list_free_list_and_data(list_dir, free);

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
		FREE(target);
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
		FREE(target_path);
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
				FREE(lostnfound);
				FREE(target);
				return ret;
			}
			FREE(lostnfound);

			base = sys_basename(file->filename);
			lostnfound = sys_path_join("%s/lost+found/%s", globals.path_prefix, base);
			/* this will fail if the directory was not already emptied */
			ret = rename(target, lostnfound);
			if (ret < 0 && errno != ENOTEMPTY && errno != EEXIST) {
				error("failed to move %s to lost+found: %s\n",
				      base, strerror(errno));
			}
			FREE(lostnfound);
		} else {
			ret = rename(file->staging, target);
			if (ret < 0) {
				error("failed to rename staged %s to final: %s\n",
				      file->hash, strerror(errno));
			}
			unlink(file->staging);
		}
	}

	FREE(target);
	return ret;
}

static int rename_all_files_to_final(struct list *updates)
{
	int ret, update_errs = 0;
	struct list *list;
	int complete = 0;
	int list_length = list_len(updates);

	list = list_head(updates);
	while (list) {
		struct file *file;
		file = list->data;
		list = list->next;

		if (!file->do_not_update) {
			ret = rename_staged_file_to_final(file);
			if (ret != 0) {
				update_errs++;
			}
		}

		complete++;
		progress_report(list_length + complete, list_length * 2);
	}

	return -update_errs;
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

enum swupd_code target_root_install_single_file(struct file *file, struct manifest *mom)
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

enum swupd_code target_root_install_files(struct list *files, struct manifest *mom)
{
	int ret = SWUPD_OK;

	if (!list_is_sorted(files, cmp_file_filename_is_deleted)) {
		UNEXPECTED();
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
	if (ret != 0) {
		ret = SWUPD_COULDNT_RENAME_FILE;
		return ret;
	}

	/* sync */
	sync();

	/*********** critical section ends ************************************/

	return SWUPD_OK;
}

/* Iterate the file list and remove from the file system each file/directory */
int target_root_remove_files(struct list *files)
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
		fullfile = sys_path_join("%s/%s", globals.path_prefix, file->filename);
		if (sys_rm_recursive(fullfile) == -1) {
			/* if a -1 is returned it means there was an issue deleting the
			 * file or directory, in that case decrease the counter of deleted
			 * files.
			 * Note: If a file didn't exist it will still be counted as deleted,
			 * this is a limitation */
			deleted--;
		}
		FREE(fullfile);
		count++;
		progress_report(count, total);
	}

	return deleted;
}
