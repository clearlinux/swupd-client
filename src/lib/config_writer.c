/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2019 Intel Corporation.
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
#include <stdio.h>
#include <unistd.h>

#include "log.h"
#include "strings.h"

int config_write_section(FILE *file, const char *section)
{
	int ret = 0;
	char *repo_string;

	if (!file) {
		return -1;
	}

	string_or_die(&repo_string, "\n[%s]\n\n", section);
	ret = fputs(repo_string, file);

	if (ret < 0 || ret == EOF) {
		error("config section write failed\n");
	} else if (ret > 0) {
		ret = 0;
	}

	free_string(&repo_string);
	return ret;
}

int config_write_config(FILE *file, const char *key, const char *value)
{
	int ret = 0;
	char *repo_string;

	if (!file) {
		return -1;
	}

	string_or_die(&repo_string, "%s=%s\n", key, value);
	ret = fputs(repo_string, file);

	if (ret < 0 || ret == EOF) {
		error("config(key,value) write failed\n");
	} else if (ret > 0) {
		ret = 0;
	}

	free_string(&repo_string);
	return ret;
}
