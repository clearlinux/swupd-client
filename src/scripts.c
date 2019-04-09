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

#define CLEAR_SERVICE_RESTART_SCRIPT "/usr/bin/clr-service-restart"

// Run script if it exists. It's a macro instead of a functions to be able
// to call another function with variable number of parameters
#define run_script_if_exists(_scriptname, ...)                                                   \
	do {                                                                                     \
		if (!file_is_executable(_scriptname)) {                                          \
			warn("helper script (%s) not found, it will be skipped\n", _scriptname); \
			break;                                                                   \
		}                                                                                \
		run_command_full(NULL, NULL, _scriptname, __VA_ARGS__);                          \
	} while (0)

static void update_boot(void)
{
	char *scriptname;

	/* Don't run clr-boot-manager update in a container on the rootfs */
	if (strcmp("/", path_prefix) == 0 && systemd_in_container()) {
		return;
	}

	if (strcmp("/", path_prefix) == 0) {
		run_script_if_exists("/usr/bin/clr-boot-manager", "update", NULL);
	} else {
		string_or_die(&scriptname, "%s/usr/bin/clr-boot-manager", path_prefix);
		run_script_if_exists(scriptname, "update", "--path", path_prefix, NULL);
		free_string(&scriptname);
	}
}

void exec_post_update_script(bool reexec, bool block)
{

	char *params[5];
	int i = 0;
	bool has_path_prefix;

	has_path_prefix = strcmp("/", path_prefix) != 0;

	params[i++] = str_or_die("%s%s", has_path_prefix ? path_prefix : "",
				 POST_UPDATE);

	if (has_path_prefix) {
		params[i++] = path_prefix;
	}

	if (block) {
		params[i++] = "--no-block";
	}

	if (reexec) {
		params[i++] = "--reexec";
	}

	params[i++] = NULL;

	run_command_params(params);
	free(params[0]);
}

static void update_triggers(bool block)
{
	if (strlen(POST_UPDATE) == 0) {
		/* fall back to systemd if path prefix is not the rootfs
		 * and the POST_UPDATE trigger wasn't specified */
		if (strcmp("/", path_prefix) != 0) {
			return;
		}

		if (!systemctl_active()) {
			warn("systemctl not operable, "
			     "unable to run systemd update triggers\n");
			return;
		}

		/* These must block so that new update triggers are executed after */
		if (need_systemd_reexec) {
			systemctl_daemon_reexec();
		} else {
			systemctl_daemon_reload();
		}

		/* Check for daemons that need to be restarted */
		if (file_is_executable(CLEAR_SERVICE_RESTART_SCRIPT)) {
			run_command(CLEAR_SERVICE_RESTART_SCRIPT, NULL);
		}

		if (block) {
			systemctl_restart("update-triggers.target");
		} else {
			systemctl_restart_noblock("update-triggers.target");
		}
	} else {
		/* These must block so that new update triggers are executed after */

		exec_post_update_script(need_systemd_reexec, block);
	}
}

void scripts_run_post_update(bool block)
{
	if (no_scripts) {
		warn("post-update helper scripts skipped due to "
		     "--no-scripts argument\n");
		return;
	}

	info("Calling post-update helper scripts.\n");

	if (need_update_boot || need_update_bootloader) {
		if (no_boot_update) {
			warn("boot files update skipped due to "
			     "--no-boot-update argument\n");
		} else {
			update_boot();
		}
	}

	update_triggers(block);
}

static void exec_pre_update_script(const char *script)
{
	if (strlen(PRE_UPDATE) == 0 || strcmp("/", path_prefix) == 0) {
		run_script_if_exists(script, NULL);
	} else {
		run_script_if_exists(script, path_prefix, NULL);
	}
}

void scripts_run_pre_update(struct manifest *manifest)
{
	struct list *iter = list_tail(manifest->files);
	struct file *file;
	char *script;

	if (strlen(PRE_UPDATE) == 0) {
		string_or_die(&script, "/usr/bin/clr_pre_update.sh");
	} else {
		string_or_die(&script, "%s/%s", path_prefix, PRE_UPDATE);
	}

	if (!file_exists(script)) {
		goto end;
	}

	// Check script hash
	while (iter != NULL) {
		file = iter->data;
		iter = iter->prev;

		if (strcmp(file->filename, script) != 0) {
			continue;
		}

		/* Check that system file matches file in manifest */
		if (verify_file(file, script)) {
			debug("Running pre-update script: %s", script);
			exec_pre_update_script(script);
			break;
		}
	}

end:
	free_string(&script);
}
