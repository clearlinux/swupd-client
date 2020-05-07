/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2020 Intel Corporation.
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
 *         Timothy C. Pepper <timothy.c.pepper@linux.intel.com>
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "swupd.h"

/* trailing slash is to indicate dir itself is expected to exist, but
 * contents are ignored */
static bool is_config(char *filename)
{
	if (str_starts_with(filename, "/etc/") == 0) {
		return true;
	}
	return false;
}

static void config_file_heuristics(struct file *file)
{
	if (is_config(file->filename)) {
		file->is_config = 1;
	}
}

/* trailing slash is to indicate dir itself is expected to exist, but
 * contents are ignored */
static bool is_state(char *filename)
{
	if (is_directory_mounted(filename)) {
		return true;
	}

	if (is_under_mounted_directory(filename)) {
		return true;
	}

	if ((str_len(filename) == 14) && (str_starts_with(filename, "/usr/src/debug") == 0)) {
		return false;
	}

	/* Compare the first part of the path, first all the entries inside
	 * kernel directory, then only the kernel directory */
	if ((str_starts_with(filename, "/usr/src/kernel/") == 0) ||
	    ((str_len(filename) == 15) && (str_starts_with(filename, "/usr/src/kernel") == 0))) {
		return false;
	}

	if ((str_starts_with(filename, "/data") == 0) ||
	    (str_starts_with(filename, "/dev/") == 0) ||
	    (str_starts_with(filename, "/home/") == 0) ||
	    (str_starts_with(filename, "/lost+found") == 0) ||
	    (str_starts_with(filename, "/proc/") == 0) ||
	    (str_starts_with(filename, "/root/") == 0) ||
	    (str_starts_with(filename, "/run/") == 0) ||
	    (str_starts_with(filename, "/sys/") == 0) ||
	    (str_starts_with(filename, "/tmp/") == 0) ||
	    (str_starts_with(filename, "/usr/src/") == 0) ||
	    (str_starts_with(filename, "/var/") == 0)) {
		return true;
	}
	return false;
}

static void runtime_state_heuristics(struct file *file)
{
	if (is_state(file->filename)) {
		file->is_state = 1;
	}
}

static void boot_file_heuristics(struct file *file)
{
	if ((str_starts_with(file->filename, "/boot/") == 0) ||
	    (str_starts_with(file->filename, "/usr/lib/modules/") == 0)) {
		file->is_boot = 1;
	}

	if (str_starts_with(file->filename, "/usr/lib/kernel/") == 0) {
		file->is_boot = 1;
		globals.need_update_bootmanager = true;
	}

	if (str_cmp(file->filename, "/usr/lib/systemd/systemd") == 0) {
		globals.need_systemd_reexec = true;
	}

	if ((str_cmp(file->filename, "/usr/lib/gummiboot") == 0) ||
	    (str_cmp(file->filename, "/usr/bin/gummiboot") == 0) ||
	    (str_cmp(file->filename, "/usr/bin/bootctl") == 0) ||
	    (str_starts_with(file->filename, "/usr/lib/systemd/boot") == 0)) {
		file->is_boot = 1;
		globals.need_update_bootmanager = true;
	}
}

static void boot_manager_heuristics(struct file *file)
{
	if ((str_cmp(file->filename, "/usr/bin/clr-boot-manager") == 0) ||
	    (str_cmp(file->filename, "/usr/share/syslinux/ldlinux.c32") == 0)) {
		globals.need_update_bootmanager = true;
	}
}

void apply_heuristics(struct file *file)
{
	runtime_state_heuristics(file);
	boot_file_heuristics(file);
	config_file_heuristics(file);
	boot_manager_heuristics(file);
}

/* Determines whether or not FILE should be ignored for this swupd action. Note
 * that boot files are ignored only if they are marked as deleted; this does
 * not happen in current manifests produced by swupd-server, but this check is
 * enabled in case swupd-server ever allows for deleted boot files in manifests
 * in the future.
 */
bool ignore(struct file *file)
{
	if ((OS_IS_STATELESS && file->is_config) ||
	    (OS_IS_STATELESS && is_config(file->filename)) || // ideally we trust the manifest but short term reapply check here
	    (file->is_state) ||
	    is_state(file->filename) || // ideally we trust the manifest but short term reapply check here
	    (file->is_boot && file->is_deleted) ||
	    (file->is_orphan) ||
	    (file->is_ghosted)) {
		file->do_not_update = 1;
		return true;
	}

	return false;
}
