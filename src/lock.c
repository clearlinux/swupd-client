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
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "swupd.h"

static int lock_fd = -1;

/* Try to get a write lock region on the lock file. Returns:
* >= 0 an fcntl region lock'd fd or exits with a positive error
* code and a recommended course of action for user.
*/
int p_lockfile(void)
{
	if (lock_fd > 0) {
		return -1;
	}

	int ret;
	pid_t pid = getpid();
	struct flock fl = {
		.l_type = F_WRLCK,
		.l_whence = SEEK_SET,
		.l_start = 0,
		.l_len = 0,
		.l_pid = pid,
	};
	char *lockfile;

	string_or_die(&lockfile, "%s/swupd_lock", state_dir);

	/* open lock file */
	lock_fd = open(lockfile, O_RDWR | O_CREAT | O_CLOEXEC, 0600);

	if (lock_fd < 0) {
		error("failed to open %s: %s\n", lockfile, strerror(errno));
		free_string(&lockfile);
		return -1;
	}

	free_string(&lockfile);

	/* try to get an advisory region lock for the file */
	ret = fcntl(lock_fd, F_SETLK, &fl);
	if (ret == -1) {
		if ((errno == EAGAIN) || (errno == EACCES)) {
			ret = -EAGAIN;
		}
		error("cannot acquire lock file. Another swupd process is already running (possibly auto-update).\n");
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
void v_lockfile(void)
{
	if (lock_fd > 0) {
		close(lock_fd);
	}
}
