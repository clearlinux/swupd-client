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
 *         Timothy C. Pepper <timothy.c.pepper@linux.intel.com>
 *         Tudor Marcu <tudor.marcu@intel.com>
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>

#include "swupd.h"

static void update_kernel(void)
{
	char *kernel_update_cmd = NULL;

	if (strcmp("/", path_prefix) == 0) {
		string_or_die(&kernel_update_cmd, "/usr/bin/kernel_updater.sh");
	} else {
		string_or_die(&kernel_update_cmd, "%s/usr/bin/kernel_updater.sh --path %s", path_prefix, path_prefix);
	}

	system(kernel_update_cmd);
	free(kernel_update_cmd);
}

static void update_bootloader(void)
{
	char *bootloader_update_cmd = NULL;

	if (strcmp("/", path_prefix) == 0) {
		string_or_die(&bootloader_update_cmd, "/usr/bin/gummiboot_updaters.sh");
	} else {
		string_or_die(&bootloader_update_cmd, "%s/usr/bin/gummiboot_updaters.sh --path %s", path_prefix, path_prefix);
	}

	system(bootloader_update_cmd);
	free(bootloader_update_cmd);

	if (strcmp("/", path_prefix) == 0) {
		string_or_die(&bootloader_update_cmd, "/usr/bin/systemdboot_updater.sh");
	} else {
		string_or_die(&bootloader_update_cmd, "%s/usr/bin/systemdboot_updater.sh --path %s", path_prefix, path_prefix);
	}

	system(bootloader_update_cmd);
	free(bootloader_update_cmd);
}

static void update_triggers(void)
{
	system("/usr/bin/systemctl daemon-reload");
	system("/usr/bin/systemctl restart update-triggers.target");
}

void run_scripts(void)
{
	printf("Calling post-update helper scripts.\n");

	/* path_prefix aware helper */
	if (need_update_boot) {
		update_kernel();
	} else {
		printf("No kernel update needed, skipping helper call out.\n");
	}

	if (need_update_bootloader) {
		update_bootloader();
	} else {
		printf("No bootloader update needed, skipping helper call out.\n");
	}

	/* helpers which don't run when path_prefix is set */
	if (strcmp("/", path_prefix) != 0) {
		return;
	}

	/* Crudely call post-update hooks after every update...FIXME */
	update_triggers();
}

/* Run any "mandatory" pre-update scripts needed. In this case, mandatory
 * means the script must run, but it is not yet fatal if the script does not
 * return success */
void run_preupdate_scripts(struct manifest *manifest)
{
	struct list *iter = list_tail(manifest->files);
	struct file *file;
	struct stat sb;
	char *script;

	string_or_die(&script, "/usr/bin/clr_pre_update.sh");

	if (stat(script, &sb) == -1) {
		free(script);
		return;
	}

	while (iter != NULL) {
		file = iter->data;
		iter = iter->prev;

		if (strcmp(file->filename, script) != 0) {
			continue;
		}

		/* Check that system file matches file in manifest */
		if (verify_file(file, script)) {
			system(script);
			break;
		}
	}

	free(script);
}
