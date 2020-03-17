/*
 *   Software Updater - client side
 *
 *      Copyright (c) 2012-2018 Intel Corporation.
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

// Make sure we are getting the gnu version of basename
#define _GNU_SOURCE
#include <libgen.h>
#undef basename
#include <string.h>

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
	output = string_join(" ", str);
	list_free_list(str);

	debug(output);
	free(output);
}

int run_command_full_params(const char *stdout_file, const char *stderr_file, char **params)
{
	int pid, ret, child_ret;
	const char *cmd = params[0];
	int null_fd;
	int pipe_fds[2];

	if (log_get_level() == LOG_DEBUG) {
		print_debug_run_command(params);
	}

	if (pipe2(pipe_fds, O_CLOEXEC) < 0) {
		error("run_command %s failed to create pipe: \n", cmd);
		return -1;
	};

	pid = fork();
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
	null_fd = open("/dev/null", O_RDONLY);
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

	exit(EXIT_FAILURE); // execve nevers return on success
	return 0;

error_child:
	exit(255);
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
	params = malloc(sizeof(char *) * (args_count + 2));
	ON_NULL_ABORT(params);

	params[i++] = (char *)cmd;

	va_start(ap, cmd);
	while ((arg = va_arg(ap, char *)) != NULL) {
		params[i++] = arg;
	}
	va_end(ap);

	params[i] = NULL;

	ret = run_command_full_params(stdout_file, stderr_file, params);

	free(params);
	return ret;
}

long get_available_space(const char *path)
{
	struct statvfs stat;

	if (statvfs(path, &stat) != 0) {
		return -1;
	}

	return stat.f_bsize * stat.f_bavail;
}

int copy_all(const char *src, const char *dst)
{
	return run_command_quiet("/bin/cp", "-a", src, dst, NULL);
}

int copy(const char *src, const char *dst)
{
	return run_command_quiet("/bin/cp", src, dst, NULL);
}

int mkdir_p(const char *dir)
{
	return run_command_quiet("/bin/mkdir", "-p", dir, NULL);
}

int rm_rf(const char *file)
{
	return run_command_quiet("/bin/rm", "-rf", file, NULL);
}

/* Get a list of files in a directory sorted by filename
 * with their fullpath, returns NULL on error (errno set by
 * opendir).
 */
struct list *get_dir_files_sorted(char *path)
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
		string_or_die(&name, "%s/%s", path, ent->d_name);
		files = list_prepend_data(files, name);
	}

	if (!ent && errno) {
		list_free_list_and_data(files, free);
		files = NULL;
	}
	(void)closedir(dir);

	return list_sort(files, strcmp_wrapper);
}

bool sys_file_exists(const char *filename)
{
	struct stat sb;

	if (lstat(filename, &sb) == 0) {
		return true;
	}

	return false;
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

	return st.st_nlink;
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
	free(tmp);
	return dir;
}

char *sys_basename(const char *path)
{
	if (!path) {
		return NULL;
	}

	return basename(path);
}

char *sys_path_join(const char *prefix, const char *path)
{
	size_t len = 0;

	if (!path) {
		path = "";
	}
	if (!prefix) {
		prefix = "";
	}

	len = strlen(prefix);

	//Remove trailing '/' at the end of prefix
	while (len && prefix[len - 1] == PATH_SEPARATOR) {
		len--;
	}

	//Remove leading  '/' at the beginning of path
	while (path[0] == PATH_SEPARATOR) {
		path++;
	}

	return str_or_die("%.*s%c%s", len, prefix, PATH_SEPARATOR, path);
}

bool is_root(void)
{
	return getuid() == 0;
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

static int sys_rm_dir_recursive(const char *path)
{
	DIR *dir;
	struct dirent *entry;
	char *filename = NULL;
	int ret = 0;

	dir = opendir(path);
	if (dir == NULL) {
		ret = -errno;
		goto exit;
	}

	while (true) {
		errno = 0;
		entry = readdir(dir);
		if (!entry) {
			break;
		}

		if (!strcmp(entry->d_name, ".") ||
		    !strcmp(entry->d_name, "..")) {
			continue;
		}

		free(filename);
		filename = str_or_die("%s/%s", path, entry->d_name);

		ret = sys_rm_recursive(filename);
		if (ret) {
			goto exit;
		}
	}

	/* Delete directory once it's empty */
	ret = sys_rmdir(path);

exit:
	closedir(dir);
	free(filename);
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
