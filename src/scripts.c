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

#include "config.h"
#include "swupd.h"

static bool in_container(void)
{
	char *detect_container_cmd = NULL;
	bool ret = false;
	/* systemd-detect-virt -c does container detection only *
         * The return code is zero if the system is in a container */
	string_or_die(&detect_container_cmd, "/usr/bin/systemd-detect-virt -c");
	ret = !system(detect_container_cmd);
	free_string(&detect_container_cmd);

	return ret;
}

static void update_boot(void)
{
	char *boot_update_cmd = NULL;
	__attribute__((unused)) int ret = 0;

	/* Don't run clr-boot-manager update in a container on the rootfs */
	if (strcmp("/", path_prefix) == 0 && in_container()) {
		return;
	}

	if (strcmp("/", path_prefix) == 0) {
		string_or_die(&boot_update_cmd, "/usr/bin/clr-boot-manager update");
	} else {
		string_or_die(&boot_update_cmd, "%s/usr/bin/clr-boot-manager update --path %s", path_prefix, path_prefix);
	}

	ret = system(boot_update_cmd);
	free_string(&boot_update_cmd);
}

static void update_triggers(bool block)
{
	char *cmd = NULL;
	char const *block_flag = NULL;
	char const *reexec_flag = NULL;
	__attribute__((unused)) int ret = 0;

	if (!block) {
		block_flag = "--no-block";
	} else {
		block_flag = "";
	}

	if (strlen(POST_UPDATE) == 0) {
		/* fall back to systemd if path prefix is not the rootfs
		 * and the POST_UPDATE trigger wasn't specified */
		if (strcmp("/", path_prefix) != 0) {
			return;
		}

		ret = system("/usr/bin/systemctl > /dev/null 2>&1");
		if (ret != 0) {
			fprintf(stderr, "WARNING: systemctl not operable, "
					"unable to run systemd update triggers\n");
			return;
		}
		/* These must block so that new update triggers are executed after */
		if (need_systemd_reexec) {
			ret = system("/usr/bin/systemctl daemon-reexec");
		} else {
			ret = system("/usr/bin/systemctl daemon-reload");
		}

		/* Check for daemons that need to be restarted */
		if (access("/usr/bin/clr-service-restart", F_OK | X_OK) == 0) {
			ret = system("/usr/bin/clr-service-restart");
		}

		string_or_die(&cmd, "/usr/bin/systemctl %s restart update-triggers.target", block_flag);
	} else {
		/* These must block so that new update triggers are executed after */
		if (need_systemd_reexec) {
			reexec_flag = "--reexec";
		} else {
			reexec_flag = "";
		}

		if (strcmp("/", path_prefix) == 0) {
			string_or_die(&cmd, "%s %s %s", POST_UPDATE, reexec_flag, block_flag);
		} else {
			string_or_die(&cmd, "%s/%s %s %s %s", path_prefix, POST_UPDATE, path_prefix, reexec_flag, block_flag);
		}
	}
	ret = system(cmd);

	free_string(&cmd);
}

void run_scripts(bool block)
{
	if (no_scripts) {
		fprintf(stderr, "WARNING: post-update helper scripts skipped due to "
				"--no-scripts argument\n");
		return;
	}

	fprintf(stderr, "Calling post-update helper scripts.\n");

	if (need_update_boot || need_update_bootloader) {
		if (no_boot_update) {
			fprintf(stderr, "WARNING: boot files update skipped due to "
					"--no-boot-update argument\n");
		} else {
			update_boot();
		}
	}

	update_triggers(block);
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
	__attribute__((unused)) int ret = 0;

	if (strlen(PRE_UPDATE) == 0) {
		string_or_die(&script, "/usr/bin/clr_pre_update.sh");
	} else {
		if (strcmp("/", path_prefix) == 0) {
			string_or_die(&script, PRE_UPDATE);
		} else {
			string_or_die(&script, "%s/%s %s", path_prefix, PRE_UPDATE, path_prefix);
		}
	}

	if (stat(script, &sb) == -1) {
		free_string(&script);
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
			ret = system(script);
			break;
		}
	}

	free_string(&script);
}
