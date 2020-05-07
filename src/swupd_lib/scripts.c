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
 *         Arjan van de Ven <arjan@linux.intel.com>
 *         Timothy C. Pepper <timothy.c.pepper@linux.intel.com>
 *         Tudor Marcu <tudor.marcu@intel.com>
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "swupd.h"

#define CLEAR_SERVICE_RESTART_SCRIPT "/usr/bin/clr-service-restart"

// Run script if it exists. It's a macro instead of a functions to be able
// to call another function with variable number of parameters
#define run_script_if_exists(_scriptname, ...)                                                   \
	do {                                                                                     \
		if (!sys_filelink_is_executable(_scriptname)) {                                  \
			warn("helper script (%s) not found, it will be skipped\n", _scriptname); \
			break;                                                                   \
		}                                                                                \
		run_command_full(NULL, NULL, _scriptname, __VA_ARGS__);                          \
	} while (0)

static void update_boot(void)
{
	char *scriptname;

	/* Don't run clr-boot-manager update in a container on the rootfs */
	if (str_cmp("/", globals.path_prefix) == 0 && systemd_in_container()) {
		return;
	}

	if (str_cmp("/", globals.path_prefix) == 0) {
		run_script_if_exists("/usr/bin/clr-boot-manager", "update", NULL);
	} else {
		scriptname = sys_path_join("%s/%s", globals.path_prefix, "/usr/bin/clr-boot-manager");
		run_script_if_exists(scriptname, "update", "--path", globals.path_prefix, NULL);
		FREE(scriptname);
	}
}

static void exec_post_update_script(bool reexec, bool block)
{

	char *params[5];
	int i = 0;
	bool has_path_prefix;

	has_path_prefix = str_cmp("/", globals.path_prefix) != 0;

	params[i++] = str_or_die("%s%s", has_path_prefix ? globals.path_prefix : "",
				 POST_UPDATE);

	if (has_path_prefix) {
		params[i++] = globals.path_prefix;
	}

	if (block) {
		params[i++] = "--no-block";
	}

	if (reexec) {
		params[i++] = "--reexec";
	}

	params[i++] = NULL;

	run_command_params(params);
	FREE(params[0]);
}

static void run_ldconfig(void)
{
	int err;

	if (str_cmp("/", globals.path_prefix) == 0) {
		err = run_command_quiet("/usr/bin/ldconfig", NULL);
	} else {
		err = run_command_quiet("/bin/chroot", globals.path_prefix, "/usr/bin/ldconfig", NULL);
	}

	if (err) {
		debug("Running ldconfig failed with error code = %d\n", err);
	}
}

static void update_triggers(bool block)
{
	if (str_len(POST_UPDATE) == 0) {
		/* fall back to systemd if path prefix is not the rootfs
		 * and the POST_UPDATE trigger wasn't specified */
		if (str_cmp("/", globals.path_prefix) != 0) {
			return;
		}

		if (!systemctl_active()) {
			warn("systemctl not operable, "
			     "unable to run systemd update triggers\n");
			return;
		}

		/* These must block so that new update triggers are executed after */
		if (globals.need_systemd_reexec) {
			systemctl_daemon_reexec();
		} else {
			systemctl_daemon_reload();
		}

		/* Check for daemons that need to be restarted */
		if (sys_filelink_is_executable(CLEAR_SERVICE_RESTART_SCRIPT)) {
			run_command(CLEAR_SERVICE_RESTART_SCRIPT, NULL);
		}

		if (block) {
			systemctl_restart("update-triggers.target");
		} else {
			systemctl_restart_noblock("update-triggers.target");
		}
	} else {
		/* These must block so that new update triggers are executed after */

		exec_post_update_script(globals.need_systemd_reexec, block);
	}
}

void scripts_run_post_update(bool block)
{
	if (globals.no_scripts) {
		warn("post-update helper scripts skipped due to "
		     "--no-scripts argument\n");
		return;
	}

	info("Calling post-update helper scripts\n");

	if (globals.need_update_bootmanager) {
		if (globals.no_boot_update) {
			warn("boot files update skipped due to "
			     "--no-boot-update argument\n");
		} else {
			update_boot();
		}
	}

	run_ldconfig();
	update_triggers(block);
}

static void exec_pre_update_script(const char *script)
{
	if (str_len(PRE_UPDATE) == 0 || str_cmp("/", globals.path_prefix) == 0) {
		run_script_if_exists(script, NULL);
	} else {
		run_script_if_exists(script, globals.path_prefix, NULL);
	}
}

void scripts_run_pre_update(struct manifest *manifest)
{
	struct list *iter = list_tail(manifest->files);
	struct file *file;
	char *script;

	if (str_len(PRE_UPDATE) == 0) {
		string_or_die(&script, "/usr/bin/clr_pre_update.sh");
	} else {
		string_or_die(&script, "%s/%s", globals.path_prefix, PRE_UPDATE);
	}

	if (!sys_file_exists(script)) {
		goto end;
	}

	// Check script hash
	while (iter != NULL) {
		file = iter->data;
		iter = iter->prev;

		if (str_cmp(file->filename, script) != 0) {
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
	FREE(script);
}
