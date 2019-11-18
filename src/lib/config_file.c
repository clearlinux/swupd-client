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

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "config_file.h"
#include "log.h"
#include "macros.h"
#include "strings.h"

#define CONFIG_LINE_MAXLEN (PATH_MAX * 2)

bool config_file_parse(const char *filename, parse_config_fn_t parse_config_fn, void *data)
{
	FILE *config_file;
	char line[CONFIG_LINE_MAXLEN];
	char *line_ptr;
	char *section = NULL;
	char *key = NULL;
	char *lkey = NULL;
	char *value = NULL;
	bool ret = true;

	config_file = fopen(filename, "rbm");
	if (config_file == NULL) {
		return false;
	}

	/* read the config file line by line */
	while (!feof(config_file)) {

		/* read next line */
		if (fgets(line, CONFIG_LINE_MAXLEN, config_file) == NULL) {
			break;
		}

		/* remove the line break from the current line */
		line_ptr = strchr(line, '\n');
		if (!line_ptr) {
			ret = false;
			goto close_and_exit;
		}
		*line_ptr = '\0';

		/* check the line to see if it is a comment (start with ; or #) */
		if (line[0] == ';' || line[0] == '#') {
			continue;
		}

		/* check the line to see if the line is a section */
		if (line[0] == '[' && *(line_ptr - 1) == ']') {
			*(line_ptr - 1) = '\0';
			free_string(&section);
			/* keys and sections in INI files are case insensitive,
			 * so we should convert them to a lower case first */
			section = str_tolower(line + 1);
			continue;
		}

		/* check the line to see if it is a key/value pair */
		line_ptr = strchr(line, '=');
		if (!line_ptr) {
			/* this is probably a blank line or something unrecognized,
			 * ignore the line */
			continue;
		}

		*line_ptr = '\0';
		key = line;
		value = line_ptr + 1;

		/* make sure we don't have empty key or value pairs */
		if (key[0] == '\0' || value[0] == '\0') {
			warn("Invalid key/value pair in the configuration file (key='%s', value='%s')\n", key, value);
			continue;
		}
		/* make sure we don't have more than one '=' per line */
		line_ptr = strchr(value, '=');
		if (line_ptr) {
			warn("Invalid key/value pair in the configuration file ('%s=%s')\n", key, value);
			continue;
		}

		/* load the configuration value read in the application */
		lkey = str_tolower(key);
		if (parse_config_fn && !parse_config_fn(section, lkey, value, data)) {
			warn("Unrecognized option '%s=%s' from section [%s] in the configuration file\n", key, value, section);
		}
		free_string(&lkey);
	}

close_and_exit:
	free_string(&section);
	fclose(config_file);
	return ret;
}

int config_write_section(FILE *file, const char *section)
{
	int ret = 0;

	if (!file) {
		return -EINVAL;
	}

	// Break line if not first item in file
	if (ftell(file) > 0) {
		fprintf(file, "\n");
	}

	ret = fprintf(file, "[%s]\n", section);

	return ret < 0 ? ret : 0;
}

int config_write_config(FILE *file, const char *key, const char *value)
{
	int ret = 0;

	if (!file) {
		return -EINVAL;
	}

	ret = fprintf(file, "%s=%s\n", key, value);

	return ret < 0 ? ret : 0;
}
