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

#ifndef DBUS_CLIENT_H
#define DBUS_CLIENT_H

#include "list.h"

typedef enum {
	DBUS_CMD_NO_ARGS,
	DBUS_CMD_SINGLE_ARG,
	DBUS_CMD_MULTIPLE_ARGS
} dbus_cmd_argv_type;

int dbus_client_call_method(const char *const method,
			    struct list *opts,
			    dbus_cmd_argv_type argv_type,
			    char *argv[]);

#endif /* DBUS_CLIENT_H */
