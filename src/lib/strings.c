/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2016 Intel Corporation.
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

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"
#include "strings.h"

char *strdup_or_die(const char *const str)
{
	char *result;

	result = strdup(str);
	ON_NULL_ABORT(result);

	return result;
}

void string_or_die(char **strp, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (vasprintf(strp, fmt, ap) < 0) {
		abort();
	}
	va_end(ap);
}

void free_string(char **s)
{
	if (s) {
		free(*s);
		*s = NULL;
	}
}

char *string_join(const char *separator, struct list *strings)
{
	char *str, *ret;
	size_t str_size = 1; // 1 for '\0'
	size_t sep_size, printed;
	struct list *i;

	if (!separator) {
		separator = "";
	}
	sep_size = strlen(separator);

	for (i = strings; i; i = i->next) {
		str_size += strlen(i->data);
		str_size += sep_size;
	}

	ret = str = malloc(str_size);
	ON_NULL_ABORT(str);
	str[0] = '\0';

	for (i = strings; i; i = i->next) {
		printed = snprintf(str, str_size, "%s%s",
				   i == strings ? "" : separator, (char *)i->data);
		if (printed >= str_size) {
			goto error; //shouldn't happen
		}
		str_size -= printed;
		str += printed;
	}

	return ret;
error:
	free(ret);
	return NULL;
}
