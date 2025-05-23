/*
 *   Software Updater - client side
 *
 *      Copyright (c) 2012-2025 Intel Corporation.
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
 */

/*
 * We rely on the result of basename provided only when GNU_SOURCE
 * is used. Making sure we are always going to have that implementation
 * to be used by sys_basename().
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <libgen.h>
#undef basename
#include <string.h>

#include "int.h"
#include "list.h"
#include "log.h"
#include "macros.h"
#include "memory.h"
#include "strings.h"
#include "sys.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <unistd.h>

static int replace_fd(int fd, const char *fd_file)
{
	int new_fd, replaced_fd;

	if (!fd_file) {
		return 0;
	}
	new_fd = open(fd_file, O_WRONLY | O_CREAT, S_IRWXU);
	if (new_fd < 0) {
		return new_fd;
	}

	replaced_fd = dup2(new_fd, fd);
	close(new_fd);

	return replaced_fd;
}

static void print_debug_run_command(char **params)
{
	struct list *str = NULL;
	char *output;
	char **p = params;

	str = list_append_data(str, "Running command - ");

	while (*p) {
		str = list_append_data(str, *p);
		p++;
	}

	str = list_head(str);
	output = str_join(" ", str);
	list_free_list(str);

	debug("%s\n", output);
	FREE(output);
}

int run_command_full_params(const char *stdout_file, const char *stderr_file, char **params)
{
	int pid, ret, child_ret;
	const char *cmd = params[0];
	int pipe_fds[2];

	if (log_get_level() == LOG_DEBUG) {
		print_debug_run_command(params);
	}

	if (pipe2(pipe_fds, O_CLOEXEC) < 0) {
		error("run_command %s failed to create pipe: \n", cmd);
		return -1;
	};

	pid = vfork();
	if (pid < 0) {
		error("run_command %s failed: %i (%s)\n", cmd, errno, strerror(errno));
		return -errno;
	}

	// Main
	if (pid > 0) {
#define MAX_BUFFER_SIZE 1024
		close(pipe_fds[1]);
		char buf[MAX_BUFFER_SIZE], *result;

		FILE *fstreamp = fdopen(pipe_fds[0], "r");

		while ((result = fgets(buf, MAX_BUFFER_SIZE, fstreamp)) != NULL) {
			info("External command: %s", result);
		}
		fclose(fstreamp);

		while ((ret = waitpid(pid, &child_ret, 0)) < 0) {
			if (errno != EINTR) {
				error("run_command %s failed: %i (%s)\n",
				      cmd, errno, strerror(errno));
				return -errno;
			}
		}

		return WEXITSTATUS(child_ret);
	}

	// Child
	close(pipe_fds[0]);
	/* replace the stdin with /dev/null. the underlying commands should never be
	 * run interactively. */
	int null_fd = open("/dev/null", O_RDONLY);
	if (null_fd < 0) {
		goto error_child;
	}
	if (dup2(null_fd, 0) < 0) {
		close(null_fd);
		goto error_child;
	}
	close(null_fd);

	/* we replace_fd to file only stdout,stderr point to a file passed to it
	 * otherwise it means we need to read it from pipe before printing to
	 * STDOUT, STDERR
	 */
	if (!stdout_file) {
		if (dup2(pipe_fds[1], STDOUT_FILENO) < 0) {
			goto error_child;
		}
	} else if (replace_fd(STDOUT_FILENO, stdout_file) < 0) {
		goto error_child;
	}

	if (!stderr_file) {
		if (dup2(pipe_fds[1], STDERR_FILENO) < 0) {
			goto error_child;
		}
	} else if (replace_fd(STDERR_FILENO, stderr_file) < 0) {
		goto error_child;
	}

	ret = execve(cmd, params, NULL);
	if (ret < 0) {
		error("run_command %s failed: %i (%s)\n", cmd, errno, strerror(errno));
	}

	_exit(EXIT_FAILURE); // execve nevers return on success
	return 0;

error_child:
	_exit(255);
	return 0;
}

int run_command_full(const char *stdout_file, const char *stderr_file, const char *cmd, ...)
{
	char **params;
	int args_count = 0, i = 0;
	va_list ap;
	char *arg;
	int ret = 0;

	va_start(ap, cmd);
	while ((arg = va_arg(ap, char *)) != NULL) {
		args_count++;
	}
	va_end(ap);

	// Create params array with space for all parameters,
	// command basename and NULL terminator
	params = malloc_or_die(sizeof(char *) * (int_to_uint(args_count) + 2));
	params[i++] = (char *)cmd;

	va_start(ap, cmd);
	while ((arg = va_arg(ap, char *)) != NULL) {
		params[i++] = arg;
	}
	va_end(ap);

	params[i] = NULL;

	ret = run_command_full_params(stdout_file, stderr_file, params);

	FREE(params);
	return ret;
}

long get_available_space(const char *path)
{
	struct statvfs stat;

	if (statvfs(path, &stat) != 0) {
		return -ENOENT;
	}

	return ulong_to_long(stat.f_bsize * stat.f_bavail);
}

int copy_all(const char *src, const char *dst)
{
	return run_command_quiet("/bin/cp", "-a", src, dst, NULL);
}

int copy(const char *src, const char *dst)
{
	return run_command_quiet("/bin/cp", "--preserve=all", "--no-dereference", src, dst, NULL);
}

int mkdir_p(const char *dir)
{
	return run_command_quiet("/bin/mkdir", "-p", dir, NULL);
}

int rm_rf(const char *file)
{
	return run_command_quiet("/bin/rm", "-rf", file, NULL);
}

struct list *sys_ls(char *path)
{
	DIR *dir = NULL;
	struct dirent *ent = NULL;
	struct list *files = NULL;

	dir = opendir(path);
	if (!dir) {
		/* caller should use errno for detecting issues */
		return NULL;
	}

	errno = 0;
	while ((ent = readdir(dir))) {
		char *name = NULL;
		if (ent->d_name[0] == '.') {
			continue;
		}
		string_or_die(&name, "%s", ent->d_name);
		files = list_prepend_data(files, name);
	}

	if (!ent && errno) {
		list_free_list_and_data(files, free);
		files = NULL;
	}
	(void)closedir(dir);

	return files;
}

bool sys_file_exists(const char *filename)
{
	struct stat sb;

	if (lstat(filename, &sb) == 0) {
		return true;
	}

	return false;
}

bool sys_path_is_absolute(const char *filename)
{
	char *abspath = NULL;
	bool ret = false;

	abspath = realpath(filename, NULL);
	if (!abspath) {
		if (errno != ENOENT) {
			error("Failed to get absolute path of %s (%s)\n", filename, strerror(errno));
		}
		ret = false;
		goto exit;
	}

	if (str_cmp(abspath, filename) == 0) {
		ret = true;
	}

exit:
	FREE(abspath);
	return ret;
}

bool sys_filelink_exists(const char *filename)
{
	struct stat sb;

	if (stat(filename, &sb) == 0) {
		return true;
	}

	return false;
}

bool sys_filelink_is_executable(const char *filename)
{
	struct stat sb;

	if (stat(filename, &sb) == 0 && (sb.st_mode & S_IXUSR)) {
		return true;
	}

	return false;
}

bool sys_file_is_hardlink(const char *file1, const char *file2)
{
	struct stat sb, sb2;

	return lstat(file1, &sb) == 0 && lstat(file2, &sb2) == 0 && sb.st_ino == sb2.st_ino;
}

long sys_file_hardlink_count(const char *file)
{
	struct stat st;

	if (lstat(file, &st)) {
		return -errno;
	}

	return ulong_to_long(st.st_nlink);
}

void journal_log_error(const char *message)
{
	if (!message) {
		return;
	}

	run_command_quiet("/usr/bin/systemd-cat", "--identifier=swupd", "--priority=err", "/bin/echo", message, NULL);
}

int systemctl_restart(const char *service)
{
	return systemctl_cmd("restart", service, NULL);
}

int systemctl_restart_noblock(const char *service)
{
	return systemctl_cmd("restart", "--no-block", service, NULL);
}

bool systemctl_active(void)
{
	/* In a container, "/usr/bin/systemctl" will return 1 with
	 * "Failed to connect to bus: No such file or directory"
	 * However /usr/bin/systemctl is-enabled ... will not fail, even when it
	 * should. Check that systemctl is working before reporting the output
	 * of is-enabled. */

	return systemctl_cmd(NULL) == 0;
}

int systemctl_daemon_reexec(void)
{
	return systemctl_cmd("daemon-reexec", NULL);
}

int systemctl_daemon_reload(void)
{
	return systemctl_cmd("daemon-reload", NULL);
}

bool systemd_in_container(void)
{
	/* systemd-detect-virt -c does container detection only *
	 * The return code is zero if the system is in a container */
	return !run_command("/usr/bin/systemd-detect-virt", "-c", NULL);
}

char *sys_dirname(const char *path)
{
	char *tmp, *dir;

	if (!path) {
		return NULL;
	}

	tmp = strdup_or_die(path);
	dir = dirname(tmp);

	if (dir == tmp) {
		return tmp;
	}

	dir = strdup_or_die(dir);
	FREE(tmp);
	return dir;
}

char *sys_basename(const char *path)
{
	if (!path) {
		return NULL;
	}

	return basename(path);
}

char *sys_path_join(const char *fmt, ...)
{
	char *path;
	va_list ap;
	size_t len, i, j;

	/* merge arguments into one path */
	va_start(ap, fmt);
	if (vasprintf(&path, fmt, ap) < 0) {
		abort();
	}
	va_end(ap);

	len = str_len(path);
	char *pretty_path = malloc_or_die(str_len(path) + 1);

	/* remove all duplicated PATH_SEPARATOR from the path */
	for (i = j = 0; i < len; i++) {
		if (path[i] == PATH_SEPARATOR && // Is separator and
						 // Next is also a separator or
		    (path[i + 1] == PATH_SEPARATOR ||
		     // Is a trailing separator, but not root
		     (path[i + 1] == '\0' && j != 0))) {
			/* duplicated PATH_SEPARATOR, throw it away */
			continue;
		}
		pretty_path[j] = path[i];
		j++;
	}
	pretty_path[j] = '\0';

	FREE(path);

	return pretty_path;
}

bool is_root(void)
{
	return getuid() == 0;
}

bool sys_is_mode(const char *path, mode_t mode)
{
	struct stat st;

	if (lstat(path, &st)) {
		return false;
	}

	return ((st.st_mode & 0777) == mode);
}

bool sys_is_dir(const char *path)
{
	struct stat st;

	if (lstat(path, &st)) {
		return false;
	}

	return S_ISDIR(st.st_mode);
}

bool sys_filelink_is_dir(const char *path)
{
	struct stat st;

	if (stat(path, &st)) {
		return false;
	}

	return S_ISDIR(st.st_mode);
}

static int sys_rm_file(const char *path)
{
	if (unlink(path) != 0) {
		return -errno;
	}

	return 0;
}

static int sys_rmdir(const char *path)
{
	if (rmdir(path) != 0) {
		return -errno;
	}

	return 0;
}

int sys_rm_dir_contents(const char *path)
{
	DIR *dir;
	struct dirent *entry;
	char *filename = NULL;
	int ret = 0;

	dir = opendir(path);
	if (dir == NULL) {
		return -errno;
	}

	while (true) {
		errno = 0;
		entry = readdir(dir);
		if (!entry) {
			break;
		}

		if (!str_cmp(entry->d_name, ".") ||
		    !str_cmp(entry->d_name, "..")) {
			continue;
		}

		filename = sys_path_join("%s/%s", path, entry->d_name);
		ret = sys_rm_recursive(filename);
		if (ret) {
			FREE(filename);
			goto exit;
		}
		FREE(filename);
	}

exit:
	closedir(dir);
	return ret;
}

static int sys_rm_dir_recursive(const char *path)
{
	int ret = 0;

	/* Delete directories content first */
	ret = sys_rm_dir_contents(path);
	if (ret) {
		goto exit;
	}

	/* Delete directory once it's empty */
	ret = sys_rmdir(path);

exit:
	return ret;
}

int sys_rm_recursive(const char *filename)
{
	int ret;
	ret = sys_rm_file(filename);
	if (ret == 0 || ret == -ENOENT) {
		return ret;
	}

	return sys_rm_dir_recursive(filename);
}

int sys_rm(const char *filename)
{
	int ret;
	ret = sys_rm_file(filename);
	if (ret == 0 || ret == -ENOENT) {
		return ret;
	}

	return sys_rmdir(filename);
}

long sys_get_file_size(const char *filename)
{
	static const int BLOCK_SIZE = 512;
	struct stat st;

	if (lstat(filename, &st) == 0) {
		return st.st_blocks * BLOCK_SIZE;
	}
	return -errno;
}

int sys_dir_is_empty(const char *path)
{
	DIR *dir;
	struct dirent *entry;
	int ret = 1;

	dir = opendir(path);
	if (dir == NULL) {
		ret = -errno;
		goto exit;
	}

	while (true) {
		entry = readdir(dir);
		if (!entry) {
			break;
		}

		if (!str_cmp(entry->d_name, ".") ||
		    !str_cmp(entry->d_name, "..")) {
			continue;
		}

		/* the directory is not empty*/
		ret = 0;
		break;
	}

exit:
	closedir(dir);
	return ret;
}

int sys_write_file(char *file, void *content, size_t content_size)
{
	FILE *fp = NULL;
	int ret = 0;

	fp = fopen(file, "w");
	if (!fp) {
		return -errno;
	}

	if (fwrite(content, content_size, 1, fp) <= 0) {
		ret = -1;
		error("There was an error writing to file %s\n", file);
	}

	fclose(fp);
	return ret;
}

void *sys_mmap_file(const char *file, size_t *file_length)
{
	struct stat st;
	int fd = -1;
	void *buffer = NULL;

	fd = open(file, O_RDONLY);
	if (fd == -1) {
		debug("Failed to open %s: %s\n", file, strerror(errno));
		goto error;
	}

	if (fstat(fd, &st) != 0) {
		debug("Failed to stat %s file\n", file);
		goto error;
	}
	*file_length = long_to_ulong(st.st_size);

	buffer = mmap(NULL, *file_length, PROT_READ, MAP_PRIVATE, fd, 0);
	if (buffer == MAP_FAILED) {
		buffer = NULL;
		debug("Failed to mmap %s content\n", file);
	}

error:
	if (fd >= 0) {
		close(fd);
	}

	return buffer;
}

void sys_mmap_free(void *buffer, size_t buffer_length)
{
	if (buffer && buffer_length > 0) {
		munmap(buffer, buffer_length);
	}
}

int link_or_rename(const char *orig, const char *dest)
{
	/* Try doing a regular rename if hardlink fails */
	if (link(orig, dest) != 0) {
		return rename(orig, dest);
	}

	return 0;
}

int link_or_copy(const char *orig, const char *dest)
{
	/* Try doing a copy if hardlink fails */
	if (link(orig, dest) != 0) {
		return copy(orig, dest);
	}

	return 0;
}

int link_or_copy_all(const char *orig, const char *dest)
{
	/* Try doing a copy if hardlink fails */
	if (link(orig, dest) != 0) {
		return copy_all(orig, dest);
	}

	return 0;
}

struct list *sys_get_mounted_directories(void)
{
	FILE *file;
	char *line = NULL;
	char *mnt;
	ssize_t ret;
	char *c;
	size_t n;
	char *ctx = NULL;
	struct list *mounted = NULL;

	file = fopen("/proc/self/mountinfo", "r");
	if (!file) {
		return NULL;
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
		mnt = strtok_r(line, " ", &ctx);
		while (mnt != NULL) {
			if (n == 4) {
				/* The "4" assumes today's mountinfo form of:
				 * 16 36 0:3 / /proc rw,relatime master:7 - proc proc rw
				 * where the fifth field is the mountpoint. */
				if (str_cmp(mnt, "/") != 0) {
					mounted = list_append_data(mounted, sys_path_join("%s", mnt));
				}

				break;
			}
			n++;
			mnt = strtok_r(NULL, " ", &ctx);
		}
		FREE(line);
	}
	FREE(line);
	fclose(file);

	return list_head(mounted);
}

char *sys_path_append_separator(const char *path)
{
	size_t len;

	if (!path) {
		return NULL;
	}

	len = str_len(path);
	if (len == 0 || path[len - 1] == '/') {
		return strdup_or_die(path);
	}

	return str_or_die("%s%c", path, PATH_SEPARATOR);
}
