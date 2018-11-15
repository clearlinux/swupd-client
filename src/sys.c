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

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <unistd.h>

static int replace_fd(int fd, const char *fd_file)
{
	int new_fd;

	if (!fd_file) {
		return 0;
	}
	new_fd = open(fd_file, O_WRONLY | O_CREAT, S_IRWXU);
	if (new_fd < 0) {
		return new_fd;
	}

	return dup2(new_fd, fd);
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
