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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "swupd.h"

static void update_boot(void)
{
	char *boot_update_cmd = NULL;
	__attribute__((unused)) int ret = 0;

	if (strcmp("/", path_prefix) == 0) {
		string_or_die(&boot_update_cmd, "/usr/bin/clr-boot-manager update");
	} else {
		string_or_die(&boot_update_cmd, "%s/usr/bin/clr-boot-manager update --path %s", path_prefix, path_prefix);
	}

	ret = system(boot_update_cmd);
	free(boot_update_cmd);
}

static void update_triggers(void)
{
	system("/usr/bin/systemctl --no-block daemon-reload");
	system("/usr/bin/systemctl --no-block restart update-triggers.target");
}

void run_scripts(void)
{
	printf("Calling post-update helper scripts.\n");

	/* path_prefix aware helper */
	if (need_update_boot || need_update_bootloader) {
		update_boot();
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
