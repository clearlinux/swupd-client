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
 *         Eric Lapuyade <eric.lapuyade@intel.com>
 *         Jim Kukunas <james.t.kukunas@linux.intel.com>
 *
 */

#define _GNU_SOURCE

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "swupd.h"

static int mklockdir(void)
{
	char *cmd;
	int ret;
	struct stat buf;

	memset(&buf, 0, sizeof(struct stat));
	ret = stat(LOCK_DIR, &buf);
	if ((ret == 0) && (S_ISDIR(buf.st_mode))) {
		return 0;
	} else if (force) {
		/* lock dir is not a directory, so with force, clean it up */
		unlink(LOCK_DIR);
	}

	string_or_die(&cmd, "mkdir -m 755 -p %s", LOCK_DIR);

	ret = system(cmd);
	free(cmd);
	if (ret) {
		return -1;
	}

	return 0;
}

/* Try to get a write lock region on the lock file. Returns:
* >= 0 an fcntl region lock'd fd or exits with a positive error
* code and a recommended course of action for user.
*/
int p_lockfile(void)
{
	int lock_fd, ret;
	pid_t pid = getpid();
	struct flock fl = {
		.l_type = F_WRLCK,
		.l_whence = SEEK_SET,
		.l_start = 0,
		.l_len = 0,
		.l_pid = pid,
	};
	char *lockfile;

	if (mklockdir() < 0) {
		printf("Error: Unable to create lock dir: '%s'\n", LOCK_DIR);
		if (!force) {
			printf("  Solution: Create manually or re-run with '--force'\n");
			printf("Operation Failed\n");
			exit(EXIT_FAILURE);
		}

		printf(" --force used. Attempting to continue\n");
	}

	string_or_die(&lockfile, "%s/swupd_lock", LOCK_DIR);

	/* open lock file */
	lock_fd = open(lockfile, O_RDWR | O_CREAT | O_CLOEXEC, 0600);
	free(lockfile);

	if (lock_fd < 0) {
		return -1;
	}

	/* try to get an advisory region lock for the file */
	ret = fcntl(lock_fd, F_SETLK, &fl);
	if (ret == -1) {
		if ((errno == EAGAIN) || (errno == EACCES)) {
			ret = -EAGAIN;
		}
		close(lock_fd);
		return ret;
	} else {
		/* speculatively dump our pid in the file,
		 * that may be useful for debug */
		ret = ftruncate(lock_fd, 0);
		ret = write(lock_fd, &pid, sizeof(pid));

		/* our lock_fd represents the lock */
		return lock_fd;
	}
}

/* closes lock fd and must not unlink lock file (else race allowed) */
void v_lockfile(int fd)
{
	close(fd);
}
