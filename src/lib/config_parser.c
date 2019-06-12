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

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "config_parser.h"
#include "log.h"
#include "macros.h"
#include "strings.h"

#define CONFIG_LINE_MAXLEN (PATH_MAX * 2)

bool config_parse(const char *filename, load_config_fn_t load_config_fn)
{
	FILE *config_file;
	char line[CONFIG_LINE_MAXLEN];
	char *line_ptr;
	char *section = NULL;
	char *key = NULL;
	char *value = NULL;
	bool ret = false;

	config_file = fopen(filename, "rbm");
	if (config_file == NULL) {
		goto exit;
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
			section = strdup_or_die(line + 1);
		}

		/* check the line to see if it is a key/value pair */
		line_ptr = strchr(line, '=');
		if (!line_ptr) {
			/* this is probably a blank line or something unrecognized,
			 * ignore the line */
			goto free_data;
		}

		*line_ptr = '\0';
		key = strdup_or_die(line);
		value = strdup_or_die(line_ptr + 1);

		/* make sure we don't have empty key or value pairs */
		if (key[0] == '\0' || value[0] == '\0') {
			warn("Invalid key/value pair in the configuration file (key='%s', value='%s')\n", key, value);
			goto free_data;
		}
		/* make sure we don't have more than one '=' per line */
		line_ptr = strchr(value, '=');
		if (line_ptr) {
			warn("Invalid key/value pair in the configuration file ('%s=%s')\n", key, value);
			goto free_data;
		}

		/* load the configuration value read in the application */
		if (!load_config_fn(section, key, value)) {
			warn("Unrecognized option '%s=%s' from section [%s] in the configuration file\n", key, value, section);
		}
	free_data:
		free_string(&key);
		free_string(&value);
	}

	/* we made it this far, config file parsed correctly */
	ret = true;

close_and_exit:
	free_string(&section);
	fclose(config_file);
exit:
	return ret;
}
