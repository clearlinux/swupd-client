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
#include "swupd-build-variant.h"
#include "swupd.h"

void check_root(void)
{
	if (getuid() != 0) {
		fprintf(stderr, "This program must be run as root..aborting.\n\n");
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

	string_or_die(&abs_path, "%s/%s", state_dir, rel_path);

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
			free_string(&filename);
			break;
		}
		free_string(&filename);
	}

	free_string(&abs_path);
	closedir(dir);

	return ret;
}

void unlink_all_staged_content(struct file *file)
{
	char *filename;

	/* downloaded tar file */
	string_or_die(&filename, "%s/download/%s.tar", state_dir, file->hash);
	unlink(filename);
	free_string(&filename);
	string_or_die(&filename, "%s/download/.%s.tar", state_dir, file->hash);
	unlink(filename);
	free_string(&filename);

	/* downloaded and un-tar'd file */
	string_or_die(&filename, "%s/staged/%s", state_dir, file->hash);
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
	swupd_rm(dirname);
	errno = 0;
	ret = stat(dirname, &sb);
	if ((ret != -1) || (errno != ENOENT)) {
		fprintf(stderr,
			"Error \"%s\" not owned by root, is not a directory, "
			"or has the wrong permissions.\n"
			"However it couldn't be deleted. stat gives error '%s'\n",
			dirname, strerror(errno));
		exit(100);
	}
	return true; /* doesn't exist now */
}

int is_populated_dir(char *dirname)
{
	int n = 0;
	struct dirent *d;
	DIR *dir = opendir(dirname);
	if (dir == NULL) {
		return 0;
	}
	while ((d = readdir(dir)) != NULL) {
		/* account for '.' and '..' */
		if (++n > 2) {
			closedir(dir);
			return 1;
		}
	}
	closedir(dir);
	return 0;
}

int copy_all(const char *src, const char *dst)
{
	char *cmd;
	string_or_die(&cmd, "cp -a %s %s 2> /dev/null", src, dst);
	int ret = system(cmd);
	free_string(&cmd);
	return ret;
}

int mkdir_p(const char *dir)
{
	char *cmd;
	string_or_die(&cmd, "umask 077 ; mkdir -p %s", dir);
	int ret = system(cmd);
	free_string(&cmd);
	return ret;
}

static int create_required_dirs(void)
{
	int ret = 0;
	unsigned int i;
	char *dir;
#define STATE_DIR_COUNT (sizeof(state_dirs) / sizeof(state_dirs[0]))
	const char *state_dirs[] = { "delta", "staged", "download", "telemetry" };

	// check for existence
	ensure_root_owned_dir(state_dir);

	for (i = 0; i < STATE_DIR_COUNT; i++) {
		string_or_die(&dir, "%s/%s", state_dir, state_dirs[i]);
		ret = ensure_root_owned_dir(dir);
		if (ret) {
			ret = mkdir_p(dir);
			if (ret) {
				fprintf(stderr, "Error: failed to create %s\n", dir);
				return -1;
			}
		}
		free_string(&dir);
	}
	/* Do a final check to make sure that the top level dir wasn't
	 * tampered with whilst we were creating the dirs */
	return ensure_root_owned_dir(state_dir);
}

/**
 * store a colon separated list of current mountpoint into
 * variable mounted_dirs, this function do not return a value.
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

				if (mounted_dirs == NULL) {
					string_or_die(&mounted_dirs, "%s", ":");
				}
				tmp = mounted_dirs;
				string_or_die(&mounted_dirs, "%s%s:", tmp, mnt);
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

// prepends prefix to an path (eg: the global path_prefix to a
// file->filename or some other path prefix and path), insuring there
// is no duplicate '/' at the strings' junction and no trailing '/'
char *mk_full_filename(const char *prefix, const char *path)
{
	char *fname = NULL;
	size_t len = 0;

	if (!path) {
		return NULL;
	}

	if (prefix) {
		len = strlen(prefix);
	}
	//Remove trailing '/' at the end of prefix
	while (len && prefix[len - 1] == '/') {
		len--;
	}

	//make sure a '/' will always be added between prefix and path
	if (path[0] == '/') {
		string_or_die(&fname, "%.*s%s", len, prefix, path);
	} else {
		string_or_die(&fname, "%.*s/%s", len, prefix, path);
	}
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
	free_string(&tmp);

	if (strstr(mounted_dirs, fname)) {
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

	if (mounted_dirs == NULL) {
		return false;
	}

	dir = strdup_or_die(mounted_dirs);

	token = strtok(dir + 1, ":");
	while (token != NULL) {
		string_or_die(&mountpoint, "%s/", token);

		tmp = mk_full_filename(path_prefix, filename);
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
	struct dirent *entry;
	char *filename = NULL;
	int ret = 0;
	int err;

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

	while (true) {
		errno = 0;
		entry = readdir(dir);
		if (!entry) {
			if (!errno) {
				ret = errno;
			}
			break;
		}

		if (!strcmp(entry->d_name, ".") ||
		    !strcmp(entry->d_name, "..")) {
			continue;
		}

		free_string(&filename);
		string_or_die(&filename, "%s/%s", path, entry->d_name);

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
	free_string(&filename);
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
	free_string(&filename);
	return ret;
}

void free_file_data(void *data)
{
	struct file *file = (struct file *)data;

	if (!file) {
		return;
	}

	/* peer and full_manifest are pointers to files contained
	 * in other lists and must not be disposed */
	free_string(&file->filename);
	free_string(&file->bundlename);
	free_string(&file->staging);

	if (file->header) {
		free(file->header);
	}

	free(file);
}

void swupd_deinit(void)
{
	terminate_signature();
	swupd_curl_deinit();
	free_globals();
	v_lockfile();
	dump_file_descriptor_leaks();
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
int swupd_init(void)
{
	int ret = 0;

	check_root();
	record_fds();

	/* Check that our system time is reasonably valid before continuing,
	 * or the certificate verification will fail with invalid time */
	if (timecheck) {
		if (system("verifytime") != 0) {
			ret = EBADTIME;
			goto out_fds;
		}
	}

	if (!init_globals()) {
		ret = EINIT_GLOBALS;
		goto out_fds;
	}

	get_mounted_directories();

	if (create_required_dirs()) {
		ret = EREQUIRED_DIRS;
		goto out_fds;
	}

	if (p_lockfile() < 0) {
		ret = ELOCK_FILE;
		goto out_fds;
	}

	if (swupd_curl_init() != 0) {
		ret = ECURL_INIT;
		goto out_close_lock;
	}

	/* If --nosigcheck, we do not attempt any signature checking */
	if (sigcheck && !initialize_signature()) {
		ret = ESIGNATURE;
		terminate_signature();
		goto out_close_lock;
	}

	return ret;

out_close_lock:
	v_lockfile();
out_fds:
	dump_file_descriptor_leaks();

	return ret;
}

/* Return a duplicated copy of the string using strdup().
 * Abort if there's no memory to allocate the new string.
*/
char *strdup_or_die(const char *const str)
{
	char *result;

	result = strdup(str);
	ON_NULL_ABORT(result);

	return result;
}

char *strndup_or_die(const char *const str, size_t size)
{
	char *result;

	result = strndup(str, size);
	ON_NULL_ABORT(result);

	return result;
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

void free_string(char **s)
{
	if (s) {
		free(*s);
		*s = NULL;
	}
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
	char *tmp = NULL;
	char *dir;
	char *real_path = NULL;

	string_or_die(&tmp, "%s", fullname);
	dir = dirname(tmp);

	fd = open(dir, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Failed to open dir %s (%s)\n", dir, strerror(errno));
		goto out;
	}

	real_path = realpath(dir, NULL);
	if (!real_path) {
		fprintf(stderr, "Failed to get real path of %s (%s)\n", dir, strerror(errno));
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

	free_string(&real_path);
out:
	free_string(&tmp);
	return ret;
}

static void free_path_data(void *data)
{
	char *path = (char *)data;
	free_string(&path);
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
	path = strdup_or_die(targetpath);
	if (path[strlen(path) - 1] == '/') {
		path[strlen(path) - 1] = '\0';
	}

	/* Breaking down the path into parts.
	 * eg. Path /usr/bin/foo will be broken into /usr,/usr/bin and /usr/bin/foo
	 */
	while (strcmp(path, "/") != 0) {
		path_list = list_prepend_data(path_list, strdup_or_die(path));
		tmp = strdup_or_die(dirname(path));
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

		target = mk_full_filename(path_prefix, path);

		/* Search for the file in the manifest, to get the hash for the file */
		file = search_file_in_manifest(target_MoM, path);
		if (file == NULL) {
			fprintf(stderr, "Error: Path %s not found in any of the subscribed manifests"
					"in verify_fix_path for path_prefix %s\n",
				path, path_prefix);
			ret = -1;
			goto end;
		}

		if (file->is_deleted) {
			fprintf(stderr, "Error: Path %s found deleted in verify_fix_path\n", path);
			ret = -1;
			goto end;
		}

		ret = stat(target, &sb);
		if (ret == 0) {
			if (verify_file(file, target)) {
				continue;
			}
			fprintf(stderr, "Hash did not match for path : %s ... fixing\n", path);
		} else if (ret == -1 && errno == ENOENT) {
			fprintf(stderr, "Path %s is missing on the file system ... fixing\n", path);
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

		string_or_die(&tar_dotfile, "%s/download/.%s.tar", state_dir, file->hash);

		string_or_die(&url, "%s/%i/files/%s.tar", content_url, file->last_change, file->hash);
		ret = swupd_curl_get_file(url, tar_dotfile);

		if (ret != 0) {
			fprintf(stderr, "Error: Failed to download file %s in verify_fix_path\n", file->filename);
			unlink(tar_dotfile);
			goto end;
		}
		if (untar_full_download(file) != 0) {
			fprintf(stderr, "Error: Failed to untar file %s\n", file->filename);
			ret = -1;
			goto end;
		}

		ret = do_staging(file, target_MoM);
		if (ret != 0) {
			fprintf(stderr, "Error: Path %s failed to stage in verify_fix_path\n", path);
			goto end;
		}
	}
end:
	free_string(&target);
	free_string(&tar_dotfile);
	free_string(&url);
	list_free_list_and_data(path_list, free_path_data);
	return ret;
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

	string_or_die(&state_v_path, "%s/version", state_dir);
	os_release_v = get_current_version(path_prefix);
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

void print_progress(unsigned int count, unsigned int max)
{
	static int last_percentage = -1;

	if (isatty(fileno(stdout))) {
		/* Only print when the percentage changes, so a maximum of 100 times per run */
		int percentage = (int)(100 * ((float)count / (float)max));
		if (percentage != last_percentage) {
			printf("\r\t...%d%%", percentage);
			fflush(stdout);
			last_percentage = percentage;
		}
	} else {
		// Print the first one, then every 10 after that
		if (count % 10 == 1) {
			printf(".");
			fflush(stdout);
		}
	}
}

/* Compare formats of the global variable (format_string) and the format number
 * from a manifest (usually a MoM). Because "staging" is treated specially and
 * never appears in a manifest, it is skipped. */
bool is_compatible_format(int format_num)
{
	if (strcmp(format_string, "staging") == 0) {
		return true;
	}

	char *format_manifest = NULL;
	string_or_die(&format_manifest, "%d", format_num);

	size_t len = strlen(format_string);

	bool ret;
	if (strncmp(format_string, format_manifest, len) == 0) {
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

	return (version == get_current_version(path_prefix));
}

/* Check if the format at default_format_path has changed from the beginning
 * of the update */
bool on_new_format(void)
{
	int res = -1;
	char *ret_str;

	res = get_value_from_path(&ret_str, DEFAULT_FORMAT_PATH, false);
	if (res != 0) {
		// could not detect current format
		return false;
	}

	res = strtoull(ret_str, NULL, 10);
	free_string(&ret_str);
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

/* list the tarfile content, and verify it contains only one line equal to the expected hash.
 * loop through all the content to detect the case where archive contains more than one file.
 */
static int check_tarfile_content(struct file *file, const char *tarfilename)
{
	int err;
	char *tarcommand;
	FILE *tar;
	int count = 0;

	string_or_die(&tarcommand, TAR_COMMAND " -tf %s/download/%s.tar 2> /dev/null", state_dir, file->hash);

	err = access(tarfilename, R_OK);
	if (err) {
		goto free_tarcommand;
	}

	tar = popen(tarcommand, "r");
	if (tar == NULL) {
		err = -1;
		goto free_tarcommand;
	}

	while (!feof(tar)) {
		char *c;
		char buffer[PATH_MAXLEN];

		if (fgets(buffer, PATH_MAXLEN, tar) == NULL) {
			if (count != 1) {
				err = -1;
			}
			break;
		}

		c = strchr(buffer, '\n');
		if (c) {
			*c = 0;
		}
		if (c && (c != buffer) && (*(c - 1) == '/')) {
			/* strip trailing '/' from directory tar */
			*(c - 1) = 0;
		}
		if (strcmp(buffer, file->hash) != 0) {
			err = -1;
			break;
		}
		count++;
	}

	pclose(tar);
free_tarcommand:
	free_string(&tarcommand);

	return err;
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

	string_or_die(&tar_dotfile, "%s/download/.%s.tar", state_dir, file->hash);
	string_or_die(&tarfile, "%s/download/%s.tar", state_dir, file->hash);
	string_or_die(&targetfile, "%s/staged/%s", state_dir, file->hash);

	/* If valid target file already exists, we're done.
	 * NOTE: this should NEVER happen given the checking that happens
	 *       ahead of queueing a download.  But... */
	if (lstat(targetfile, &stat) == 0) {
		if (verify_file(file, targetfile)) {
			unlink(tar_dotfile);
			unlink(tarfile);
			free_string(&tar_dotfile);
			free_string(&tarfile);
			free_string(&targetfile);
			return 0;
		} else {
			unlink(tarfile);
			unlink(targetfile);
		}
	} else if (lstat(tarfile, &stat) == 0) {
		/* remove tar file from possible past failure */
		unlink(tarfile);
	}

	err = rename(tar_dotfile, tarfile);
	if (err) {
		free_string(&tar_dotfile);
		goto exit;
	}
	free_string(&tar_dotfile);

	err = check_tarfile_content(file, tarfile);
	if (err) {
		goto exit;
	}

	/* modern tar will automatically determine the compression type used */
	char *outputdir;
	string_or_die(&outputdir, "%s/staged", state_dir);
	err = extract_to(tarfile, outputdir);
	free_string(&outputdir);
	if (err) {
		fprintf(stderr, "ignoring tar extract failure for fullfile %s.tar (ret %d)\n",
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
		fprintf(stderr, "Error: File content hash mismatch for %s (bad server data?)\n", targetfile);
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
