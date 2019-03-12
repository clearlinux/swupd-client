/*
 *   Software Updater - client side
 *
 *      Copyright © 2019 Intel Corporation.
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

#define _GNU_SOURCE

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "swupd.h"

/*
 * Create a list of parameters compatible with execv().
 *
 * By definition:
 *  - The first parameter is the name of the binary that is being run.
 *  - Followed by each parameter in argv as a string in the params array.
 *  - Terminated with a NULL pointer.
 */
static char **create_params_list(char *binary_name, int argc, char **argv)
{
	char **params = malloc(sizeof(char *) * (argc + 2));
	int i;

	params[0] = binary_name;
	for (i = 0; i < argc; i++) {
		params[i + 1] = argv[i];
	}
	params[argc + 1] = NULL;

	return params;
}

enum swupd_code binary_loader_main(int argc, char **argv)
{
	char *binary_name;
	char *full_binary_path;
	char *command;
	char **params = NULL;
	int ret = SWUPD_OK;

	if (argc == 0) {
		return SWUPD_INVALID_OPTION;
	}

	command = argv[0];
	string_or_die(&binary_name, "swupd-%s", command);
	string_or_die(&full_binary_path, "/usr/bin/%s", binary_name);

	if (!file_is_executable(full_binary_path)) {
		error("Binary %s doesn't exist in your system\n", full_binary_path);
		ret = SWUPD_INVALID_BINARY;
		goto end;
	}

	params = create_params_list(binary_name, argc - 1, argv + 1);
	ret = execv(full_binary_path, params);
	if (ret < 0) {
		error("Running %s failed\n", binary_name);
		ret = SWUPD_SUBPROCESS_ERROR;
	}

end:
	free(params);
	free_string(&binary_name);
	free_string(&full_binary_path);

	return ret;
}
