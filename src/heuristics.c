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
	if (strncmp(filename, "/etc/", 5) == 0) {
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

	if ((str_len(filename) == 14) && (strncmp(filename, "/usr/src/debug", 14) == 0)) {
		return false;
	}

	/* Compare the first part of the path, first all the entries inside
	 * kernel directory, then only the kernel directory */
	if ((strncmp(filename, "/usr/src/kernel/", 16) == 0) ||
	    ((str_len(filename) == 15) && (strncmp(filename, "/usr/src/kernel", 15) == 0))) {
		return false;
	}

	if ((strncmp(filename, "/data", 5) == 0) ||
	    (strncmp(filename, "/dev/", 5) == 0) ||
	    (strncmp(filename, "/home/", 6) == 0) ||
	    (strncmp(filename, "/lost+found", 11) == 0) ||
	    (strncmp(filename, "/proc/", 6) == 0) ||
	    (strncmp(filename, "/root/", 6) == 0) ||
	    (strncmp(filename, "/run/", 5) == 0) ||
	    (strncmp(filename, "/sys/", 5) == 0) ||
	    (strncmp(filename, "/tmp/", 5) == 0) ||
	    (strncmp(filename, "/usr/src/", 9) == 0) ||
	    (strncmp(filename, "/var/", 5) == 0)) {
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
	if ((strncmp(file->filename, "/boot/", 6) == 0) ||
	    (strncmp(file->filename, "/usr/lib/modules/", 17) == 0)) {
		file->is_boot = 1;
	}

	if (strncmp(file->filename, "/usr/lib/kernel/", 16) == 0) {
		file->is_boot = 1;
		globals.need_update_boot = true;
	}

	if (strcmp(file->filename, "/usr/lib/systemd/systemd") == 0) {
		globals.need_systemd_reexec = true;
	}

	if ((strncmp(file->filename, "/usr/lib/gummiboot", 18) == 0) ||
	    (strncmp(file->filename, "/usr/bin/gummiboot", 18) == 0) ||
	    (strncmp(file->filename, "/usr/bin/bootctl", 16) == 0) ||
	    (strncmp(file->filename, "/usr/lib/systemd/boot", 21) == 0)) {
		file->is_boot = 1;
		globals.need_update_bootloader = true;
	}
}

static void boot_manager_heuristics(struct file *file)
{
	if ((strcmp(file->filename, "/usr/bin/clr-boot-manager") == 0) ||
	    (strcmp(file->filename, "/usr/share/syslinux/ldlinux.c32") == 0)) {
		globals.need_update_bootloader = true;
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
		globals.update_skip++;
		file->do_not_update = 1;
		return true;
	}

	return false;
}
