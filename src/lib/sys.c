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

#include "sys.h"
#include "list.h"
#include "log.h"
#include "macros.h"
#include "memory.h"
#include "strings.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int run_command_full_params(const char *stdout_file, const char *stderr_file, char **params)
{
	int pid, ret, child_ret;
	const char *cmd = params[0];
	int null_fd;

	pid = fork();
	if (pid < 0) {
		error("run_command %s failed: %i (%s)\n", cmd, errno, strerror(errno));
		return -errno;
	}

	// Main
	if (pid > 0) {
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

	/* replace the stdin with /dev/null. the underlying commands should never be
	 * run interactively. */
	null_fd = open("/dev/null", O_RDONLY);
	dup2(null_fd, 0);
	close(null_fd);

	if (replace_fd(STDOUT_FILENO, stdout_file) < 0) {
		goto error_child;
	}

	if (replace_fd(STDERR_FILENO, stderr_file) < 0) {
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

static int lex_sort(const void *a, const void *b)
{
	const char *name1 = (char *)a;
	const char *name2 = (char *)b;
	return strcmp(name1, name2);
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

	return list_sort(files, lex_sort);
}

bool file_exists(const char *filename)
{
	struct stat sb;

	if (stat(filename, &sb) == 0) {
		return true;
	}

	return false;
}

bool file_is_executable(const char *filename)
{
	struct stat sb;

	if (stat(filename, &sb) == 0 && (sb.st_mode & S_IXUSR)) {
		return true;
	}

	return false;
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
	return !run_command("/usr/bin/systemd-detect-virt", "-c");
}

char *sys_dirname(const char *path)
{
	char *tmp, *dir;

	tmp = strdup_or_die(path);
	dir = strdup_or_die(dirname(tmp));

	free(tmp);
	return dir;
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

	if (prefix) {
		len = strlen(prefix);
	}

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
