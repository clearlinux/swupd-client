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
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

#include "config.h"
#include "signature.h"
#include "swupd.h"
#include "swupd_build_variant.h"
#include "verifytime.h"

void check_root(void)
{
	if (!is_root()) {
		error("This program must be run as root..aborting\n\n");
		exit(EXIT_FAILURE);
	}
}

/* Remove the contents of a staging directory (eg: /mnt/swupd/update/780 or
 * /mnt/swupd/update/delta) which are not supposed to contain
 * subdirectories containing files, ie: no need for true recursive removal.
 * Just the relative path (et: "780" or "delta" is passed as a parameter).
 *
 * return: 0 on success, non-zero on error
 */
int rm_staging_dir_contents(const char *rel_path)
{
	DIR *dir;
	struct dirent *entry;
	char *filename;
	char *abs_path;
	int ret = 0;

	string_or_die(&abs_path, "%s/%s", globals.state_dir, rel_path);

	dir = opendir(abs_path);
	if (dir == NULL) {
		free_string(&abs_path);
		return -1;
	}

	while (true) {
		errno = 0;
		entry = readdir(dir);
		if (!entry) {
			/* readdir returns NULL on the end of a directory stream, we only
			 * want to set ret if errno is also set, indicating a failure */
			if (errno) {
				ret = errno;
			}
			break;
		}

		if (!strcmp(entry->d_name, ".") ||
		    !strcmp(entry->d_name, "..")) {
			continue;
		}

		string_or_die(&filename, "%s/%s", abs_path, entry->d_name);

		ret = remove(filename);
		if (ret != 0) {
			debug("Failed to remove file %s\n", filename);
			free_string(&filename);
			break;
		}
		free_string(&filename);
	}

	free_string(&abs_path);
	closedir(dir);

	return ret;
}

static void unlink_all_staged_content(struct file *file)
{
	char *filename;

	/* downloaded tar file */
	string_or_die(&filename, "%s/download/%s.tar", globals.state_dir, file->hash);
	unlink(filename);
	free_string(&filename);
	string_or_die(&filename, "%s/download/.%s.tar", globals.state_dir, file->hash);
	unlink(filename);
	free_string(&filename);

	/* downloaded and un-tar'd file */
	string_or_die(&filename, "%s/staged/%s", globals.state_dir, file->hash);
	(void)remove(filename);
	free_string(&filename);
}

/* Ensure that a directory either doesn't exist
 * or does exist and belongs to root.
 * If it exists, but is not a directory or belonging to root, remove it
 * returns true if it doesn't exist
 */
static int ensure_root_owned_dir(const char *dirname)
{
	struct stat sb;
	int ret = stat(dirname, &sb);
	if (ret && (errno == ENOENT)) {
		return true;
	}
	if ((sb.st_uid == 0) &&
	    (S_ISDIR(sb.st_mode)) &&
	    ((sb.st_mode & 0777) == 0700)) {
		return false;
	}
	/* Oops, not owned by root or
	 * not a directory or wrong perms
	 */
	ret = sys_rm_recursive(dirname);
	if (ret != 0 && ret != -ENOENT) {
		error(
		    "Error \"%s\" not owned by root, is not a directory, "
		    "or has the wrong permissions\n"
		    "However it couldn't be deleted. stat gives error '%s'\n",
		    dirname, strerror(errno));
		exit(100);
	}
	return true; /* doesn't exist now */
}

static bool is_populated_dir(const char *dirname)
{
	int n = 0;
	struct dirent *d;
	DIR *dir = opendir(dirname);
	if (dir == NULL) {
		return false;
	}
	while ((d = readdir(dir)) != NULL) {
		/* account for '.' and '..' */
		if (++n > 2) {
			closedir(dir);
			return true;
		}
	}
	closedir(dir);
	return false;
}

static bool validate_tracking_dir(const char *state_dir)
{
	int ret = 0;
	char *src;
	char *tracking_dir;
	char *rmfile;

	tracking_dir = sys_path_join(state_dir, "bundles");

	/* if state_dir_parent/bundles doesn't exist or is empty, assume this is
	 * the first time tracking installed bundles. Since we don't know what the
	 * user installed themselves just copy the entire system tracking directory
	 * into the state tracking directory. */
	if (!is_populated_dir(tracking_dir)) {
		src = sys_path_join(globals.path_prefix, "/usr/share/clear/bundles");

		if (!sys_file_exists(src)) {
			/* there is no bundles directory to copy from */
			free_string(&src);
			free_string(&tracking_dir);
			return false;
		}

		ret = rm_rf(tracking_dir);
		if (ret) {
			goto out;
		}

		/* at the point this function is called <bundle_name> is already
		 * installed on the system and therefore has a tracking file under
		 * /usr/share/clear/bundles. A simple cp -a of that directory will
		 * accurately track that bundle as manually installed. */
		ret = copy_all(src, state_dir);
		free_string(&src);
		if (ret) {
			goto out;
		}

		/* remove uglies that live in the system tracking directory */
		rmfile = sys_path_join(tracking_dir, ".MoM");
		(void)unlink(rmfile);
		free_string(&rmfile);

		/* set perms on the directory correctly */
		ret = chmod(tracking_dir, S_IRWXU);
		if (ret) {
			goto out;
		}
	}
out:
	free_string(&tracking_dir);
	if (ret) {
		return false;
	}
	return true;
}

int create_state_dirs(const char *state_dir_path)
{
	int ret = 0;
	unsigned int i;
	char *dir;
#define STATE_DIR_COUNT (sizeof(state_dirs) / sizeof(state_dirs[0]))
	const char *state_dirs[] = { "delta", "staged", "download", "telemetry", "bundles", "3rd_party" };

	// check for existence
	if (ensure_root_owned_dir(state_dir_path)) {
		// state dir doesn't exist
		if (mkdir_p(state_dir_path) != 0 || chmod(state_dir_path, S_IRWXU) != 0) {
			error("failed to create %s\n", state_dir_path);
			return -1;
		}
	}

	for (i = 0; i < STATE_DIR_COUNT; i++) {
		string_or_die(&dir, "%s/%s", state_dir_path, state_dirs[i]);
		ret = ensure_root_owned_dir(dir);
		if (ret) {
			ret = mkdir(dir, S_IRWXU);
			if (ret) {
				error("failed to create %s\n", dir);
				free_string(&dir);
				return -1;
			}
		}
		free_string(&dir);
	}
	/* Do a final check to make sure that the top level dir wasn't
	 * tampered with whilst we were creating the dirs */
	if (ensure_root_owned_dir(state_dir_path)) {
		return -1;
	}

	/* make sure the tracking directory is not empty, if it is,
	 * mark all installed bundles as tracked */
	if (!validate_tracking_dir(state_dir_path)) {
		debug("There was an error accessing the tracking directory %s/bundles\n", state_dir_path);
	}

	return ret;
}

/**
 * store a colon separated list of current mountpoint into
 * variable globals.mounted_dirs, this function do not return a value.
 *
 * e.g: :/proc:/mnt/acct:
 */
static void get_mounted_directories(void)
{
	FILE *file;
	char *line = NULL;
	char *mnt;
	char *tmp;
	ssize_t ret;
	char *c;
	size_t n;

	file = fopen("/proc/self/mountinfo", "r");
	if (!file) {
		return;
	}

	while (!feof(file)) {
		ret = getline(&line, &n, file);
		if ((ret < 0) || (line == NULL)) {
			break;
		}

		c = strchr(line, '\n');
		if (c) {
			*c = 0;
		}

		n = 0;
		mnt = strtok(line, " ");
		while (mnt != NULL) {
			if (n == 4) {
				/* The "4" assumes today's mountinfo form of:
				* 16 36 0:3 / /proc rw,relatime master:7 - proc proc rw
				* where the fifth field is the mountpoint. */
				if (strcmp(mnt, "/") == 0) {
					break;
				}

				if (globals.mounted_dirs == NULL) {
					string_or_die(&globals.mounted_dirs, "%s", ":");
				}
				tmp = globals.mounted_dirs;
				string_or_die(&globals.mounted_dirs, "%s%s:", tmp, mnt);
				free_string(&tmp);
				break;
			}
			n++;
			mnt = strtok(NULL, " ");
		}
		free_string(&line);
	}
	free_string(&line);
	fclose(file);
}

static int get_version_from_path(const char *abs_path)
{
	int ret = -1;
	int val;
	char *ret_str;

	ret = get_value_from_path(&ret_str, abs_path, true);
	if (ret == 0) {
		int err = strtoi_err(ret_str, &val);
		free_string(&ret_str);

		if (err != 0) {
			error("Invalid version\n");
		} else {
			return val;
		}
	}

	return -1;
}

// expects filename w/o path_prefix prepended
bool is_directory_mounted(const char *filename)
{
	char *fname;
	bool ret = false;
	char *tmp;

	if (globals.mounted_dirs == NULL) {
		return false;
	}

	tmp = sys_path_join(globals.path_prefix, filename);
	string_or_die(&fname, ":%s:", tmp);
	free_string(&tmp);

	if (strstr(globals.mounted_dirs, fname)) {
		ret = true;
	}

	free_string(&fname);

	return ret;
}

// expects filename w/o path_prefix prepended
bool is_under_mounted_directory(const char *filename)
{
	bool ret = false;
	int err;
	char *token;
	char *mountpoint;
	char *dir;
	char *fname;
	char *tmp;

	if (globals.mounted_dirs == NULL) {
		return false;
	}

	dir = strdup_or_die(globals.mounted_dirs);

	token = strtok(dir + 1, ":");
	while (token != NULL) {
		string_or_die(&mountpoint, "%s/", token);

		tmp = sys_path_join(globals.path_prefix, filename);
		string_or_die(&fname, ":%s:", tmp);
		free_string(&tmp);

		err = strncmp(fname, mountpoint, strlen(mountpoint));
		free_string(&fname);
		if (err == 0) {
			free_string(&mountpoint);
			ret = true;
			break;
		}

		token = strtok(NULL, ":");

		free_string(&mountpoint);
	}

	free_string(&dir);

	return ret;
}

void free_file_data(void *data)
{
	struct file *file = (struct file *)data;

	if (!file) {
		return;
	}

	/* peer is a pointer to file contained
	 * in another list and must not be disposed */
	free_string(&file->filename);
	free_string(&file->staging);

	if (file->header) {
		free(file->header);
	}

	free(file);
}

void swupd_deinit(void)
{
	swupd_curl_cleanup();
	signature_deinit();
	v_lockfile();
	globals_deinit();
	dump_file_descriptor_leaks();
}

void remove_trailing_slash(char *url)
{
	int len = strlen(url);

	while (len > 0 && url[len - 1] == '/') {
		len--;
		url[len] = 0;
	}
}

/* this function is intended to encapsulate the basic swupd
* initializations for the majority of commands, that is:
* 	- Make sure root is the user running the code
* 	- Initialize globals
*	- initialize mounted directories
*	- Create necessary directories
*	- Get the lock
*	- Initialize curl
*	- Initialize signature checking
*/
enum swupd_code swupd_init(enum swupd_init_config config)
{
	enum swupd_code ret = SWUPD_OK;

	record_fds();

	if (!globals_init()) {
		ret = SWUPD_INIT_GLOBALS_FAILED;
		goto out_fds;
	}

	get_mounted_directories();

	if ((config & SWUPD_NO_ROOT) == 0) {
		check_root();

		/* Check that our system time is reasonably valid before continuing,
		 * or the certificate verification will fail with invalid time */
		if ((config & SWUPD_NO_TIMECHECK) == 0 && globals.timecheck) {
			if (!verify_time(globals.path_prefix)) {
				/* in the case we are doing an installation to an empty directory
				 * using swupd verify --install, we won't have a valid versionstamp
				 * in path_prefix, so try searching without the path_prefix */
				if (!verify_time(NULL)) {
					ret = SWUPD_BAD_TIME;
					goto out_fds;
				}
			}
		}

		if (create_state_dirs(globals.state_dir)) {
			ret = SWUPD_COULDNT_CREATE_DIR;
			goto out_fds;
		}

		if (globals.state_dir_cache != NULL) {
			if (create_state_dirs(globals.state_dir_cache)) {
				ret = SWUPD_COULDNT_CREATE_DIR;
				goto out_fds;
			}
		}

		if (p_lockfile() < 0) {
			ret = SWUPD_LOCK_FILE_FAILED;
			goto out_fds;
		}
	}

	if (globals.sigcheck) {
		/* If --nosigcheck, we do not attempt any signature checking */
		if (!signature_init(globals.cert_path, NULL)) {
			ret = SWUPD_SIGNATURE_VERIFICATION_FAILED;
			signature_deinit();
			goto out_close_lock;
		}
	}

	return ret;

out_close_lock:
	v_lockfile();
out_fds:
	dump_file_descriptor_leaks();

	/* This means there was a problem after global_init earlier.
	 * This is needed to prevent memory leak
	 */
	globals_deinit();

	return ret;
}

void update_motd(int new_release)
{
	FILE *motd_fp = NULL;

	motd_fp = fopen(MOTD_FILE, "w");

	if (motd_fp != NULL) {
		fprintf(motd_fp, "There is a new OS version available: %d\n", new_release);
		fprintf(motd_fp, "Upgrade to the latest version using 'swupd update'\n");
		fclose(motd_fp);
	}
}

void delete_motd(void)
{
	unlink(MOTD_FILE);
}

int get_dirfd_path(const char *fullname)
{
	int ret = -1;
	int fd;
	char *dir = NULL;
	char *real_path = NULL;

	dir = sys_dirname(fullname);

	fd = open(dir, O_RDONLY);
	if (fd < 0) {
		error("Failed to open dir %s (%s)\n", dir, strerror(errno));
		goto out;
	}

	real_path = realpath(dir, NULL);
	if (!real_path) {
		error("Failed to get real path of %s (%s)\n", dir, strerror(errno));
		close(fd);
		goto out;
	}

	if (strcmp(real_path, dir) != 0) {
		/* The path to the file contains a symlink, skip the file,
		 * because we cannot safely determine if it can be deleted. */
		ret = -2;
		close(fd);
	} else {
		ret = fd;
	}

	free_string(&real_path);
out:
	free_string(&dir);
	return ret;
}

/* This function is meant to be called while staging file to fix any missing/incorrect paths.
 * While staging a file, if its parent directory is missing, this would try to create the path
 * by breaking it into sub-paths and fixing them top down.
 * Here, target_MoM is the consolidated manifest for the version you are trying to update/verify.
 */
enum swupd_code verify_fix_path(char *targetpath, struct manifest *target_MoM)
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
	if (path[strlen(path) - 1] == '/') {
		path[strlen(path) - 1] = '\0';
	}

	/* Breaking down the path into parts.
	 * eg. Path /usr/bin/foo will be broken into /usr,/usr/bin and /usr/bin/foo
	 */
	while (strcmp(path, "/") != 0) {
		path_list = list_prepend_data(path_list, strdup_or_die(path));
		tmp = sys_dirname(path);
		free_string(&path);
		path = tmp;
	}
	free_string(&path);

	list1 = list_head(path_list);
	while (list1) {
		path = list1->data;
		list1 = list1->next;

		free_string(&target);
		free_string(&tar_dotfile);
		free_string(&url);

		target = sys_path_join(globals.path_prefix, path);

		/* Search for the file in the manifest, to get the hash for the file */
		file = search_file_in_manifest(target_MoM, path);
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
			info("Hash did not match for path : %s ... fixing\n", path);
		} else if (ret == -1 && errno == ENOENT) {
			info("Path %s is missing on the file system ... fixing\n", path);
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
			error("Failed to download file %s in verify_fix_path\n", file->filename);
			ret = SWUPD_COULDNT_DOWNLOAD_FILE;
			unlink(tar_dotfile);
			goto end;
		}

		if (untar_full_download(file) != 0) {
			error("Failed to untar file %s\n", file->filename);
			ret = SWUPD_COULDNT_UNTAR_FILE;
			goto end;
		}

		ret = do_staging(file, target_MoM);
		if (ret != 0) {
			/* do_staging returns a swupd_code on error,
			* just propagate the error */
			error("Path %s failed to stage in verify_fix_path\n", path);
			goto end;
		}
	}
end:
	free_string(&target);
	free_string(&tar_dotfile);
	free_string(&url);
	list_free_list_and_data(path_list, free);
	return ret;
}

struct list *consolidate_files_from_bundles(struct list *bundles)
{
	/* bundles is a pointer to a list of manifests */
	struct list *files = NULL;

	files = files_from_bundles(bundles);
	files = consolidate_files(files);
	return files;
}

struct list *files_from_bundles(struct list *bundles)
{
	struct list *files = NULL;
	struct list *list;
	struct manifest *bundle;

	list = list_head(bundles);
	while (list) {
		struct list *bfiles;
		bundle = list->data;
		list = list->next;
		if (!bundle) {
			continue;
		}
		bfiles = list_clone(bundle->files);
		files = list_concat(bfiles, files);
	}

	return files;
}

bool version_files_consistent(void)
{
	/*
	 * Compare version numbers in state_dir/version and /usr/lib/os-release.
	 * The version in /usr/lib/os-release must match or be greater than
	 * state_dir/version for successful updates (greater than occurs during
	 * format bumps).
	 */
	int os_release_v = -1;
	int state_v = -1;
	char *state_v_path;

	string_or_die(&state_v_path, "%s/version", globals.state_dir);
	os_release_v = get_current_version(globals.path_prefix);
	state_v = get_version_from_path(state_v_path);
	free_string(&state_v_path);

	// -1 returns indicate failures
	if (os_release_v < 0 || state_v < 0) {
		return false;
	}

	return (os_release_v >= state_v);
}

bool string_in_list(char *string_to_check, struct list *list_to_check)
{
	struct list *iter;

	iter = list_head(list_to_check);
	while (iter) {
		if (strncmp(iter->data, string_to_check, strlen(string_to_check)) == 0) {
			return true;
		}

		iter = iter->next;
	}

	return false;
}

/* Compare formats of the global variable (format_string) and the format number
 * from a manifest (usually a MoM). Because "staging" is treated specially and
 * never appears in a manifest, it is skipped. */
bool is_compatible_format(int format_num)
{
	if (strcmp(globals.format_string, "staging") == 0) {
		return true;
	}

	char *format_manifest = NULL;
	string_or_die(&format_manifest, "%d", format_num);

	size_t len = strlen(globals.format_string);

	bool ret;
	if (strncmp(globals.format_string, format_manifest, len) == 0) {
		ret = true;
	} else {
		ret = false;
	}

	free_string(&format_manifest);
	return ret;
}

/* Check if the version argument is the same version as the current OS */
bool is_current_version(int version)
{
	if (version < 0) {
		return false;
	}

	return (version == get_current_version(globals.path_prefix));
}

/* Check if the format at default_format_path has changed from the beginning
 * of the update */
bool on_new_format(void)
{
	int res = -1;
	int err;
	char *ret_str;

	res = get_value_from_path(&ret_str, DEFAULT_FORMAT_PATH, false);
	if (res != 0) {
		// could not detect current format
		return false;
	}

	err = strtoi_err(ret_str, &res);
	free_string(&ret_str);

	if (err != 0) {
		return false;
	}

	/* is_compatible_format returns true if the argument matches the
	 * format_string, which was read at the beginning of the update. Return the
	 * opposite of is_compatible_format to indicate when the new format is
	 * different from the one we started on. */
	return !is_compatible_format(res);
}

/* Try to create a link to a file. If it fails, rename it.
 * Return 0 on success or an error code on errors.*/
int link_or_rename(const char *orig, const char *dest)
{
	/* Try doing a regular rename if hardlink fails */
	if (link(orig, dest) != 0) {
		return rename(orig, dest);
	}

	return 0;
}

/* Try to create a link to a file. If it fails, copy it.
 * Return 0 on success or an error code on errors.*/
int link_or_copy(const char *orig, const char *dest)
{
	/* Try doing a copy if hardlink fails */
	if (link(orig, dest) != 0) {
		return copy(orig, dest);
	}

	return 0;
}

/* Try to create a link to a file. If it fails, copy it with cp -a.
 * Return 0 on success or an error code on errors.*/
int link_or_copy_all(const char *orig, const char *dest)
{
	/* Try doing a copy if hardlink fails */
	if (link(orig, dest) != 0) {
		return copy_all(orig, dest);
	}

	return 0;
}

/* This function will break if the same HASH.tar full file is downloaded
 * multiple times in parallel. */
int untar_full_download(void *data)
{
	struct file *file = data;
	char *tarfile;
	char *tar_dotfile;
	char *targetfile;
	struct stat stat;
	int err;

	string_or_die(&tar_dotfile, "%s/download/.%s.tar", globals.state_dir, file->hash);
	string_or_die(&tarfile, "%s/download/%s.tar", globals.state_dir, file->hash);
	string_or_die(&targetfile, "%s/staged/%s", globals.state_dir, file->hash);

	/* If valid target file already exists, we're done.
	 * NOTE: this should NEVER happen given the checking that happens
	 *       ahead of queueing a download.  But... */
	if (verify_file(file, targetfile)) {
		unlink(tar_dotfile);
		unlink(tarfile);
		free_string(&tar_dotfile);
		free_string(&tarfile);
		free_string(&targetfile);
		return 0;
	}

	unlink(tarfile);
	unlink(targetfile);

	err = rename(tar_dotfile, tarfile);
	if (err) {
		free_string(&tar_dotfile);
		goto exit;
	}
	free_string(&tar_dotfile);

	err = archives_check_single_file_tarball(tarfile, file->hash);
	if (err) {
		goto exit;
	}

	/* modern tar will automatically determine the compression type used */
	char *outputdir;
	string_or_die(&outputdir, "%s/staged", globals.state_dir);
	err = archives_extract_to(tarfile, outputdir);
	free_string(&outputdir);
	if (err) {
		warn("ignoring tar extract failure for fullfile %s.tar (ret %d)\n",
		     file->hash, err);
		goto exit;
		/* TODO: respond to ARCHIVE_RETRY error codes
		 * libarchive returns ARCHIVE_RETRY when tar extraction fails but the
		 * operation is retry-able. We need to determine if it is worth our time
		 * to retry in these situations. */
	} else {
		/* Only unlink when tar succeeded, so we can examine the tar file
		 * in the failure case. */
		unlink(tarfile);
	}

	err = lstat(targetfile, &stat);
	if (!err && !verify_file(file, targetfile)) {
		/* Download was successful but the hash was bad. This is fatal*/
		error("File content hash mismatch for %s (bad server data?)\n", targetfile);
		exit(EXIT_FAILURE);
	}

exit:
	free_string(&tarfile);
	free_string(&targetfile);
	if (err) {
		unlink_all_staged_content(file);
	}
	return err;
}

/* Appends the (experimental) label if applicable to the bundle name */
char *get_printable_bundle_name(const char *bundle_name, bool is_experimental)
{
	char *printable_name = NULL;

	string_or_die(&printable_name, "%s%s", bundle_name, is_experimental ? " (experimental)" : "");
	return printable_name;
}

/* Helper to print regexp errors */
void print_regexp_error(int errcode, regex_t *regexp)
{
	size_t len;
	char *error_buffer = NULL;

	len = regerror(errcode, regexp, NULL, 0);
	error_buffer = malloc(len);
	ON_NULL_ABORT(error_buffer);

	regerror(errcode, regexp, error_buffer, len);
	error("Invalid regular expression: %s\n", error_buffer)

	    free(error_buffer);
}

bool is_url_insecure(const char *url)
{
	if (url && strncasecmp(url, "http://", 7) == 0) {
		return true;
	}

	return false;
}

bool is_url_allowed(const char *url)
{
	static bool print_warning = true;

	if (is_url_insecure(url)) {
		if (globals.allow_insecure_http) {
			if (print_warning) {
				warn("This is an insecure connection\n");
				info("The --allow-insecure-http flag was used, be aware that this poses a threat to the system\n\n");
				print_warning = false;
			}
		} else {
			if (print_warning) {
				error("This is an insecure connection\n");
				info("Use the --allow-insecure-http flag to proceed\n");
				print_warning = false;
			}
			return false;
		}
	}

	return true;
}

int get_value_from_path(char **contents, const char *path, bool is_abs_path)
{
	char line[LINE_MAX];
	FILE *file;
	char *c;
	char *rel_path;
	int ret = -1;

	/* When path is an absolute path, do not prepend the path_prefix. This
	 * happens when this function is called with state_dir as the path, which
	 * already has the path_prefix included */
	if (is_abs_path) {
		string_or_die(&rel_path, path);
	} else {
		string_or_die(&rel_path, "%s%s", globals.path_prefix, path);
	}

	file = fopen(rel_path, "r");
	if (!file) {
		free_string(&rel_path);
		return ret;
	}

	/* the file should contain exactly one line */
	line[0] = 0;
	if (fgets(line, LINE_MAX, file) == NULL) {
		if (ferror(file)) {
			error("Unable to read data from %s\n", rel_path);
		} else if (feof(file)) {
			error("Contents of %s are empty\n", rel_path);
		}
		goto fail;
	}

	/* remove newline if present */
	c = strchr(line, '\n');
	if (c) {
		*c = '\0';
	}

	string_or_die(contents, line);
	ret = 0;
fail:
	fclose(file);
	free_string(&rel_path);
	return ret;
}

/* Once system is on mix this file should exist */
bool system_on_mix(void)
{
	bool ret = (access(MIXED_FILE, R_OK) == 0);
	return ret;
}

/* If the MIX_BUNDLES_DIR has the valid-mix flag file we can run through
 * adding the mix data to the OS */
bool check_mix_exists(void)
{
	char *fullpath;
	bool ret;
	string_or_die(&fullpath, "%s%s/.valid-mix", globals.path_prefix, MIX_DIR);
	ret = access(fullpath, F_OK) == 0;
	free_string(&fullpath);
	return ret;
}

/* Iterate the file list and remove from the file system each file/directory */
int remove_files_from_fs(struct list *files)
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
		free_string(&fullfile);
		count++;
		progress_report(count, total);
	}

	return deleted;
}

void print_pattern(const char *pattern, int times)
{
	while (times > 0) {
		info("%s", pattern);
		times--;
	}
	info("\n");
}

void print_header(const char *header)
{
	int header_length;

	header_length = strlen(header);
	print_pattern("_", header_length + 1);
	info("%s\n", header);
	print_pattern("_", header_length + 1);
	info("\n");
}

void prettify_size(long size_in_bytes, char **pretty_size)
{
	double size;

	/* find the best way to show the provided size */
	size = (double)size_in_bytes;
	if (size < 1000) {
		string_or_die(pretty_size, "%.2lf Bytes", size);
		return;
	}
	size = size / 1000;
	if (size < 1000) {
		string_or_die(pretty_size, "%.2lf KB", size);
		return;
	}
	size = size / 1000;
	if (size < 1000) {
		string_or_die(pretty_size, "%.2lf MB", size);
		return;
	}
	size = size / 1000;
	string_or_die(pretty_size, "%.2lf GB", size);
}

char *get_tracking_dir(void)
{
	return sys_path_join(globals.state_dir, "bundles");
}
