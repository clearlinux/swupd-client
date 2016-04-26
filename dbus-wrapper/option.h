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

#ifndef OPTION_H
#define OPTION_H

#include <stdbool.h>

typedef enum {
	TYPE_STRING,
	TYPE_BOOL,
	TYPE_INT
} option_type_t;

typedef union _option_value {
        char *as_str;
	bool as_bool;
	int as_int;
} option_value_t;

typedef struct _command_option {
	char *name;
	option_type_t type;
	option_value_t value;
} command_option_t;

command_option_t *construct_command_option(const char *name, option_type_t type, void *value);
void free_command_option(void *data);


#endif /* OPTION_H */
