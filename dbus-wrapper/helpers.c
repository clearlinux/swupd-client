/*
 *   Software Updater - D-Bus client for the daemon controlling
 *                      Clear Linux Software Update Client.
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
 *   Contact: Dmitry Rozhkov <dmitry.rozhkov@intel.com>
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include "helpers.h"

static bool is_valid_integer_format(const char *str)
{
        unsigned long long int version;
        errno = 0;

        version = strtoull(str, NULL, 10);
        if ((errno < 0) || (version == 0)) {
                return false;
        }

        return true;
}

bool is_format_correct(const char *userinput)
{
	if (userinput == NULL) {
		return false;
	}

	// allow "staging" as a format string
	if ((strcmp(userinput, "staging") == 0)) {
		return true;
	}

	// otherwise, expect a positive integer
	if (!is_valid_integer_format(userinput)) {
		return false;
	}

        return true;
}

bool is_statedir_correct(const char *path)
{
	if (!path) {
		printf("statedir can't be empty\n");
		return false;
	}

	if (path[0] != '/') {
		printf("statedir must be a full path starting with '/', not '%c'\n", path[0]);
		return false;
	}

	return true;
}
