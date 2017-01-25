
/*
 *   Software Updater - telemetry collection
 *
 *      Copyright © 2016 Intel Corporation.
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
 *         Auke Kok <auke-jan.h.kok@intel.com>
 *
 */

#define _GNU_SOURCE
#include <libgen.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "swupd.h"

#define RECORD_VERSION 2

void telemetry(telem_prio_t level, const char *class, const char *fmt, ...)
{
	va_list args;
	char *filename;
	char *filename_n;
	char *newname;
	int fd;

	string_or_die(&filename, "%s/%d.%s.%d.XXXXXX", state_dir,
		      RECORD_VERSION, class, level);

	fd = mkstemp(filename);
	if (fd < 0) {
		free(filename);
		return;
	}

	dprintf(fd, "PACKAGE_NAME=%s\nPACKAGE_VERSION=%s\n",
		PACKAGE_NAME, PACKAGE_VERSION);

	va_start(args, fmt);
	vdprintf(fd, fmt, args);
	va_end(args);

	close(fd);

	filename_n = basename(filename);
	if (!filename_n) {
		free(filename);
		return;
	}

	string_or_die(&newname, "%s/telemetry/%s", state_dir, filename_n);

	rename(filename, newname);
	free(filename);
	free(newname);
}
