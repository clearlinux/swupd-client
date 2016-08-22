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
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "signature.h"
#include "swupd.h"

void check_root(void)
{
	if (getuid() != 0) {
		printf("This program must be run as root..aborting.\n\n");
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
	struct dirent entry;
	struct dirent *result;
	char *filename;
	char *abs_path;
	int ret;

	string_or_die(&abs_path, "%s/%s", state_dir, rel_path);

	dir = opendir(abs_path);
	if (dir == NULL) {
		free(abs_path);
		return -1;
	}

	while ((ret = readdir_r(dir, &entry, &result)) == 0) {
		if (result == NULL) {
			break;
		}

		if (!strcmp(entry.d_name, ".") ||
		    !strcmp(entry.d_name, "..")) {
			continue;
		}

		string_or_die(&filename, "%s/%s", abs_path, entry.d_name);

		ret = remove(filename);
		if (ret != 0) {
			free(filename);
			break;
		}
		free(filename);
	}

	free(abs_path);
	closedir(dir);

	sync();
	return ret;
}

void unlink_all_staged_content(struct file *file)
{
	char *filename;

	/* downloaded tar file */
	string_or_die(&filename, "%s/download/%s.tar", state_dir, file->hash);
	unlink(filename);
	free(filename);
	string_or_die(&filename, "%s/download/.%s.tar", state_dir, file->hash);
	unlink(filename);
	free(filename);

	/* downloaded and un-tar'd file */
	string_or_die(&filename, "%s/staged/%s", state_dir, file->hash);
	remove(filename);
	free(filename);

	/* delta file */
	if (file->peer) {
		string_or_die(&filename, "%s/delta/%i-%i-%s", state_dir,
			      file->peer->last_change, file->last_change, file->hash);
		unlink(filename);
		free(filename);
	}
}

FILE *fopen_exclusive(const char *filename) /* no mode, opens for write only */
{
	int fd;

	fd = open(filename, O_CREAT | O_EXCL | O_RDWR, 00600);
	if (fd < 0) {
		return NULL;
	}
	return fdopen(fd, "w");
}

int create_required_dirs(void)
{
	int ret = 0;
	int i;
	char *dir;
#define STATE_DIR_COUNT 3
	const char *dirs[] = { "delta", "staged", "download" };
	struct stat buf;
	bool missing = false;

	// check for existance
	ret = stat(state_dir, &buf);
	if (ret && (errno == ENOENT)) {
		missing = true;
	}
	for (i = 0; i < STATE_DIR_COUNT; i++) {
		string_or_die(&dir, "%s/%s", state_dir, dirs[i]);
		ret = stat(dir, &buf);
		if (ret) {
			missing = true;
		}
		free(dir);
	}

	if (missing) { // (re)create dirs
		char *cmd;

		for (i = 0; i < STATE_DIR_COUNT; i++) {
			string_or_die(&cmd, "mkdir -p %s/%s", state_dir, dirs[i]);
			ret = system(cmd);
			if (ret) {
				printf("Error: failed to create %s/%s\n", state_dir, dirs[i]);
				return -1;
			}
			free(cmd);

			string_or_die(&dir, "%s/%s", state_dir, dirs[i]);
			ret = chmod(dir, S_IRWXU);
			if (ret) {
				printf("Error: failed to set mode for %s/%s\n", state_dir, dirs[i]);
				return -1;
			}
			free(dir);
		}

		// chmod 700
		ret = chmod(state_dir, S_IRWXU);
		if (ret) {
			printf("Error: failed to set mode for state dir (%s)\n", state_dir);
			return -1;
		}
	}

	return 0;
}

/**
 * store a colon separated list of current mountpoint into
 * variable mounted_dirs, this function do not return a value.
 *
 * e.g: :/proc:/mnt/acct:
 */
void get_mounted_directories(void)
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

				if (mounted_dirs == NULL) {
					string_or_die(&mounted_dirs, "%s", ":");
				}
				tmp = mounted_dirs;
				string_or_die(&mounted_dirs, "%s%s:", tmp, mnt);
				free(tmp);
				break;
			}
			n++;
			mnt = strtok(NULL, " ");
		}
		free(line);
		line = NULL;
	}
	free(line);
	fclose(file);
}

// prepends prefix to an path (eg: the global path_prefix to a
// file->filename or some other path prefix and path), insuring there
// is no duplicate '/' at the strings' junction and no trailing '/'
char *mk_full_filename(const char *prefix, const char *path)
{
	char *fname = NULL;
	char *abspath;

	if (path[0] == '/') {
		abspath = strdup(path);
	} else {
		string_or_die(&abspath, "/%s", path);
	}
	if (abspath == NULL) {
		abort();
	}

	// The prefix is a minimum of "/" or "".  If the prefix is only that,
	// just use abspath.  If the prefix is longer than the minimal, insure
	// it ends in not "/" and append abspath.
	if ((strcmp(prefix, "/") == 0) ||
	    (strcmp(prefix, "") == 0)) {
		// rootfs, use absolute path
		fname = strdup(abspath);
		if (fname == NULL) {
			abort();
		}
	} else if (strcmp(&prefix[strlen(prefix) - 1], "/") == 0) {
		// chroot and need to strip trailing "/" from prefix
		char *tmp = strdup(prefix);
		if (tmp == NULL) {
			abort();
		}
		tmp[strlen(tmp) - 1] = '\0';

		string_or_die(&fname, "%s%s", tmp, abspath);
		free(tmp);
	} else {
		// chroot and no need to strip trailing "/" from prefix
		string_or_die(&fname, "%s%s", prefix, abspath);
	}
	free(abspath);
	return fname;
}

// expects filename w/o path_prefix prepended
bool is_directory_mounted(const char *filename)
{
	char *fname;
	bool ret = false;
	char *tmp;

	if (mounted_dirs == NULL) {
		return false;
	}

	tmp = mk_full_filename(path_prefix, filename);
	string_or_die(&fname, ":%s:", tmp);
	free(tmp);

	if (strstr(mounted_dirs, fname)) {
		ret = true;
	}

	free(fname);

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

	if (mounted_dirs == NULL) {
		return false;
	}

	dir = strdup(mounted_dirs);
	if (dir == NULL) {
		abort();
	}

	token = strtok(dir + 1, ":");
	while (token != NULL) {
		string_or_die(&mountpoint, "%s/", token);

		tmp = mk_full_filename(path_prefix, filename);
		string_or_die(&fname, ":%s:", tmp);
		free(tmp);

		err = strncmp(fname, mountpoint, strlen(mountpoint));
		free(fname);
		if (err == 0) {
			free(mountpoint);
			ret = true;
			break;
		}

		token = strtok(NULL, ":");

		free(mountpoint);
	}

	free(dir);

	return ret;
}

static int swupd_rm_file(const char *path)
{
	int err = unlink(path);
	if (err) {
		if (errno == ENOENT) {
			return 0;
		} else {
			return -1;
		}
	}
	return 0;
}

static int swupd_rm_dir(const char *path)
{
	DIR *dir;
	struct dirent entry;
	struct dirent *result;
	char *filename = NULL;
	int ret, err;

	dir = opendir(path);
	if (dir == NULL) {
		if (errno == ENOENT) {
			ret = 0;
			goto exit;
		} else {
			ret = -1;
			goto exit;
		}
	}

	while ((ret = readdir_r(dir, &entry, &result)) == 0) {
		if (result == NULL) {
			break;
		}

		if (!strcmp(entry.d_name, ".") ||
		    !strcmp(entry.d_name, "..")) {
			continue;
		}

		free(filename);
		string_or_die(&filename, "%s/%s", path, entry.d_name);

		err = swupd_rm(filename);
		if (err) {
			ret = -1;
			goto exit;
		}
	}

	/* Delete directory once it's empty */
	err = rmdir(path);
	if (err) {
		if (errno == ENOENT) {
		} else {
			ret = -1;
			goto exit;
		}
	}

exit:
	closedir(dir);
	free(filename);
	return ret;
}

int swupd_rm(const char *filename)
{
	struct stat stat;
	int ret;

	ret = lstat(filename, &stat);
	if (ret) {
		if (errno == ENOENT) {
			// Quiet, no real failure here
			return -ENOENT;
		} else {
			return -1;
		}
	}

	if (S_ISDIR(stat.st_mode)) {
		ret = swupd_rm_dir(filename);
	} else {
		ret = swupd_rm_file(filename);
	}
	return ret;
}

int rm_bundle_file(const char *bundle)
{
	char *filename = NULL;
	int ret = 0;
	struct stat statb;

	string_or_die(&filename, "%s/%s/%s", path_prefix, BUNDLES_DIR, bundle);

	if (stat(filename, &statb) == -1) {
		goto out;
	}

	if (S_ISREG(statb.st_mode)) {
		if (unlink(filename) != 0) {
			ret = -1;
			goto out;
		}
	} else {
		ret = -1;
		goto out;
	}

out:
	free(filename);
	return ret;
}

#if 0
void dump_file_info(struct file *file)
{
	printf("%s:\n", file->filename);
	printf("\t%s\n", file->hash);
	printf("\t%d\n", file->last_change);

	if (file->use_xattrs) {
		printf("\tuse_xattrs\n");
	}
	if (file->is_dir) {
		printf("\tis_dir\n");
	}
	if (file->is_file) {
		printf("\tis_file\n");
	}
	if (file->is_link) {
		printf("\tis_link\n");
	}
	if (file->is_deleted) {
		printf("\tis_deleted\n");
	}
	if (file->is_manifest) {
		printf("\tis_manifest\n");
	}
	if (file->is_config) {
		printf("\tis_config\n");
	}
	if (file->is_state) {
		printf("\tis_state\n");
	}
	if (file->is_boot) {
		printf("\tis_boot\n");
	}
	if (file->is_rename) {
		printf("\tis_rename\n");
	}
	if (file->is_orphan) {
		printf("\tis_orphan\n");
	}
	if (file->do_not_update) {
		printf("\tdo_not_update\n");
	}

	if (file->peer) {
		printf("\tpeer %s(%s)\n", file->peer->filename, file->peer->hash);
	}
	if (file->deltapeer) {
		printf("\tdeltapeer %s(%s)\n", file->deltapeer->filename, file->deltapeer->hash);
	}
	if (file->staging) {
		printf("\tstaging %s\n", file->staging);
	}
}
#endif

void free_file_data(void *data)
{
	struct file *file = (struct file *)data;

	if (!file) {
		return;
	}

	/* peer and deltapeer are pointers to files contained
	 * in another list and must not be disposed */

	if (file->filename) {
		free(file->filename);
	}

	if (file->header) {
		free(file->header);
	}

	if (file->staging) {
		free(file->staging);
	}

	free(file);
}

void swupd_deinit(int lock_fd)
{
	terminate_signature();
	swupd_curl_cleanup();
	free_subscriptions();
	free_globals();
	v_lockfile(lock_fd);
	dump_file_descriptor_leaks();
}

/* this function is intended to encapsulate the basic swupd
* initializations for the majority of commands, that is:
* 	- Make sure root is the user running the code
*	- Initialize log facility
*	- Get the lock
*	- initialize mounted directories
*	- Initialize curl
*/
int swupd_init(int *lock_fd)
{
	int ret = 0;

	check_root();

	if (!init_globals()) {
		ret = EINIT_GLOBALS;
		goto out_fds;
	}

	get_mounted_directories();

	if (create_required_dirs()) {
		ret = EREQUIRED_DIRS;
		goto out_fds;
	}

	*lock_fd = p_lockfile();
	if (*lock_fd < 0) {
		ret = ELOCK_FILE;
		goto out_fds;
	}

	if (swupd_curl_init() != 0) {
		ret = ECURL_INIT;
		goto out_close_lock;
	}

	if (!initialize_signature()) {
		terminate_signature();
		return false;
	}

	return ret;

out_close_lock:
	v_lockfile(*lock_fd);
out_fds:
	dump_file_descriptor_leaks();

	return ret;
}

/* this function prints the initial message for all utils
*/
void copyright_header(const char *name)
{
	printf(PACKAGE " %s " VERSION "\n", name);
	printf("   Copyright (C) 2012-2016 Intel Corporation\n");
	printf("\n");
}

void string_or_die(char **strp, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (vasprintf(strp, fmt, ap) < 0) {
		abort();
	}
	va_end(ap);
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
	if (unlink(MOTD_FILE) == ENOMEM) {
		abort();
	}
}

int get_dirfd_path(const char *fullname)
{
	int ret = -1;
	int fd;
	char *tmp = NULL;
	char *dir;
	char *real_path = NULL;

	string_or_die(&tmp, "%s", fullname);
	dir = dirname(tmp);

	fd = open(dir, O_RDONLY);
	if (fd < 0) {
		printf("Failed to open dir %s (%s)\n", dir, strerror(errno));
		goto out;
	}

	real_path = realpath(dir, NULL);
	if (!real_path) {
		printf("Failed to get real path of %s (%s)\n", dir, strerror(errno));
		close(fd);
		goto out;
	}

	if (strcmp(real_path, dir) != 0) {
		/* FIXME: Should instead check to see if real_path is marked
		 * non-deleted in the consolidated manifest. If it is
		 * non-deleted, then we must not delete the file (and also not
		 * flag an error); otherwise, it can be safely deleted. Until
		 * that check is implemented, always skip the file, because we
		 * cannot safely determine if it can be deleted. */
		ret = -1;
		close(fd);
	} else {
		ret = fd;
	}

	free(real_path);
out:
	free(tmp);
	return ret;
}

void free_path_data(void *data)
{
	char *path = (char *)data;
	free(path);
}

/* This function is meant to be called while staging file to fix any missing/incorrect paths.
 * While staging a file, if its parent directory is missing, this would try to create the path
 * by breaking it into sub-paths and fixing them top down.
 * Here, target_MoM is the consolidated manifest for the version you are trying to update/verify.
 */
int verify_fix_path(char *targetpath, struct manifest *target_MoM)
{
	struct list *path_list = NULL; /* path_list contains the subparts in the path */
	char *path;
	char *tmp = NULL, *target = NULL;
	char *url = NULL;
	struct stat sb;
	int ret = 0;
	struct file *file;
	char *tar_dotfile = NULL;
	struct list *list1 = NULL;

	/* This shouldn't happen */
	if (strcmp(targetpath, "/") == 0) {
		return ret;
	}

	/* Removing trailing '/' from the path */
	path = strdup(targetpath);
	if (path[strlen(path) - 1] == '/') {
		path[strlen(path) - 1] = '\0';
	}

	/* Breaking down the path into parts.
	 * eg. Path /usr/bin/foo will be broken into /usr,/usr/bin and /usr/bin/foo
	 */
	while (strcmp(path, "/") != 0) {
		path_list = list_prepend_data(path_list, strdup(path));
		tmp = strdup(dirname(path));
		free(path);
		path = tmp;
	}
	free(path);

	list1 = list_head(path_list);
	while (list1) {
		path = list1->data;
		list1 = list1->next;

		target = mk_full_filename(path_prefix, path);

		/* Search for the file in the manifest, to get the hash for the file */
		file = search_file_in_manifest(target_MoM, path);
		if (file == NULL) {
			printf("Error: Path %s not found in any of the subscribed manifests"
			       "in verify_fix_path for path_prefix %s\n",
			       path, path_prefix);
			ret = -1;
			goto end;
		}

		if (file->is_deleted) {
			printf("Error: Path %s found deleted in verify_fix_path\n", path);
			ret = -1;
			goto end;
		}

		ret = stat(target, &sb);
		if (ret == 0) {
			if (verify_file(file, target)) {
				continue;
			}
			printf("Hash did not match for path : %s\n", path);
		} else if (ret == -1 && errno == ENOENT) {
			printf("Path %s is missing on the file system\n", path);
		} else {
			goto end;
		}

		string_or_die(&tar_dotfile, "%s/download/.%s.tar", state_dir, file->hash);

		// clean up in case any prior download failed in a partial state
		unlink(tar_dotfile);

		string_or_die(&url, "%s/%i/files/%s.tar", content_url, file->last_change, file->hash);
		ret = swupd_curl_get_file(url, tar_dotfile, NULL, NULL, false);

		if (ret != 0) {
			printf("Error: Failed to download file %s in verify_fix_path\n", file->filename);
			unlink(tar_dotfile);
			goto end;
		}
		if (untar_full_download(file) != 0) {
			printf("Error: Failed to untar file %s\n", file->filename);
			ret = -1;
			goto end;
		}

		ret = do_staging(file, target_MoM);
		if (ret != 0) {
			printf("Error: Path %s failed to stage in verify_fix_path\n", path);
			goto end;
		}
	}
end:
	if (target) {
		free(target);
	}
	if (tar_dotfile) {
		free(tar_dotfile);
	}
	if (url) {
		free(url);
	}
	list_free_list_and_data(path_list, free_path_data);
	return ret;
}

/* In case both version_url and content_url have the file:// prefix, use
 * libcurl's file protocol to copy files from the local filesystem instead of
 * downloading from a web server.  The local_download global variable defaults
 * to false and is used to determine how to handle libcurl results after a
 * curl_*_perform.
 */
void set_local_download(void)
{
	bool version_local = false;
	bool content_local = false;

	if (strncmp(version_url, "file://", 7) == 0) {
		version_local = true;
	}
	if (strncmp(content_url, "file://", 7) == 0) {
		content_local = true;
	}

	/* protocols must match for download logic */
	if (version_local != content_local) {
		printf("\nVersion and content URLs must both use the HTTP/HTTPS protocols"
		       " or the libcurl FILE protocol.\n");
		exit(EXIT_FAILURE);
	}

	if (version_local && content_local) {
		local_download = true;
	}

	return;
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
