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

int run_command_full(const char *stdout_file, const char *stderr_file, const char *cmd, ...)
{
	int pid, ret, child_ret;

	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "run_command %s failed: %i (%s)\n", cmd, errno, strerror(errno));
		return -errno;
	}

	// Main
	if (pid > 0) {
		while ((ret = waitpid(pid, &child_ret, 0)) < 0) {
			if (errno != EINTR) {
				fprintf(stderr, "run_command %s failed: %i (%s)\n",
					cmd, errno, strerror(errno));
				return -errno;
			}
		}

		return WEXITSTATUS(child_ret);
	}

	// Child
	va_list ap;
	char *short_cmd;
	char **params;
	int args_count = 0, i = 0;
	char *arg;

	va_start(ap, cmd);
	while ((arg = va_arg(ap, char *)) != NULL) {
		args_count++;
	}
	va_end(ap);

	// Create params array with space for all parameters,
	// command basename and NULL terminator
	params = malloc(sizeof(char *) * (args_count + 2));
	if (!params) {
		goto error_child;
	}

	// We need a copy because of basename
	short_cmd = strdup(cmd);
	if (!short_cmd) {
		abort();
	}

	params[i++] = basename(short_cmd);

	va_start(ap, cmd);
	while ((arg = va_arg(ap, char *)) != NULL) {
		params[i++] = arg;
	}
	va_end(ap);

	params[i] = NULL;

	if (replace_fd(STDOUT_FILENO, stdout_file) < 0) {
		goto error_child;
	}

	if (replace_fd(STDERR_FILENO, stderr_file) < 0) {
		goto error_child;
	}

	ret = execve(cmd, params, NULL);
	if (ret < 0) {
		fprintf(stderr, "run_command %s failed: %i (%s)\n", cmd, errno, strerror(errno));
	}
	free(params);
	free(short_cmd);

	exit(EXIT_FAILURE); // execve nevers return on success
	return 0;

error_child:
	exit(255);
	return 0;
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

bool file_exits(const char *filename)
{
	struct stat sb;

	if (stat(filename, &sb) == 0) {
		return true;
	}

	return false;
}
