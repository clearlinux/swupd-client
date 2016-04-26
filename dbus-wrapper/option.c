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

#include <stdlib.h>
#include <string.h>

#include "option.h"
#include "log.h"

command_option_t *construct_command_option(const char *name, option_type_t type, void *value)
{
	command_option_t *option = malloc(sizeof(command_option_t));
	option->name = strdup(name);
	option->type = type;

	switch (type) {
	case TYPE_STRING:
		option->value.as_str = strdup((char *) value);
		break;
	case TYPE_BOOL:
		option->value.as_bool = * (bool*) value;
		break;
	case TYPE_INT:
		option->value.as_int = * (int*) value;
		break;
	default:
		ERR("Wrong option type");
		abort();
	}
	return option;
}

void free_command_option(void *data)
{
	command_option_t *opt = data;
	if (opt->type == TYPE_STRING) {
		free(opt->value.as_str);
	}
	free(opt->name);
	free(opt);
}
