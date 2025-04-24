/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2025 Intel Corporation.
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
 */

#include "swupd.h"

#define is_from_global_section(_section) (str_cmp(_section, "global") == 0)
#define is_from_command_section(_section, _command) (str_cmp(_section, _command) == 0)

bool config_loader_set_opt(char *section, char *opt, char *value, void *data)
{
	struct config_loader_data *cl = data;
	const struct option *options;
	char *flag = NULL;
	bool ret = false;

	if (!data) {
		error("Invalid parameter to config_loader_set_opt()\n");
		return false;
	}

	/* make sure the config loader has been initialized */
	if (!cl->command || !cl->available_opts || !cl->parse_global_opt) {
		error("Configuration loader not initialized\n");
		return false;
	}
	options = cl->available_opts;

	/* options within a section are by default considered to be global */
	if (section) {
		if (!is_from_global_section(section) && !is_from_command_section(section, cl->command)) {
			/* values being parsed are from a command not currently
			 * running, we are not interested in these */
			ret = true;
			goto exit;
		}
	}

	/* replace all '_' used in config options with '-' used in flags */
	flag = str_subchar(opt, '_', '-');

	/* search the option from within the available options */
	while (options->name != NULL) {
		if (str_cmp(flag, options->name) == 0) {
			/* if it was not a long option try looking at the global short
			 * options first... */
			ret = cl->parse_global_opt(options->val, value);
			if (ret) {
				/* global option set */
				break;
			}

			/* now try with the local short options...
			 * not all commands support local options */
			if (cl->parse_command_opt) {
				ret = cl->parse_command_opt(options->val, value);
			}

			break;
		}
		options++;
	}

exit:
	FREE(flag);
	return ret;
}
