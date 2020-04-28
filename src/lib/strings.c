/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2020 Intel Corporation.
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

#include <ctype.h>
#include <errno.h>
#include <limits.h>
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

char *vstr_or_die(const char *fmt, va_list ap)
{
	char *str;

	if (vasprintf(&str, fmt, ap) < 0) {
		abort();
	}

	return str;
}

void string_or_die(char **strp, const char *fmt, ...)
{
	va_list ap;

	if (!strp) {
		return;
	}

	va_start(ap, fmt);
	*strp = vstr_or_die(fmt, ap);
	va_end(ap);
}

char *str_or_die(const char *fmt, ...)
{
	char *str;
	va_list ap;

	va_start(ap, fmt);
	str = vstr_or_die(fmt, ap);
	va_end(ap);

	return str;
}

char *str_join(const char *separator, struct list *strings)
{
	char *str, *ret;
	size_t str_size = 1; // 1 for '\0'
	size_t sep_size, printed;
	struct list *i;

	if (!separator) {
		separator = "";
	}
	sep_size = str_len(separator);

	for (i = strings; i; i = i->next) {
		str_size += str_len(i->data);
		str_size += sep_size;
	}

	ret = str = malloc_or_die(str_size);
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
	FREE(ret);
	return NULL;
}

struct list *str_split(const char *separator, const char *string_to_split)
{
	char *ctx = NULL;
	struct list *split = NULL;
	char *string_copy = strdup_or_die(string_to_split);
	char *token = strtok_r(string_copy, separator, &ctx);
	while (token) {
		split = list_prepend_data(split, strdup_or_die(token));
		token = strtok_r(NULL, separator, &ctx);
	}
	FREE(string_copy);
	return split;
}

int str_to_int_endptr(const char *str, char **endptr, int *value)
{
	long num;
	int err;

	errno = 0;
	num = strtol(str, endptr, 10);
	err = -errno;

	/* When the return value of strtol overflows the int type, don't overflow
	 * and return an overflow error code. */
	if (num > INT_MAX) {
		num = INT_MAX;
		err = -ERANGE;
	} else if (num < INT_MIN) {
		num = INT_MIN;
		err = -ERANGE;
	}

	*value = (int)num;

	return err;
}

int str_to_int(const char *str, int *value)
{
	char *endptr;
	int err = str_to_int_endptr(str, &endptr, value);

	if (err) {
		return err;
	}

	if (*endptr != '\0' && !isspace(*endptr)) {
		return -EINVAL;
	}

	return 0;
}

int str_to_uint_endptr(const char *str, char **endptr, unsigned int *value)
{
	long num;
	int err;

	errno = 0;
	num = strtol(str, endptr, 10);
	err = -errno;

	/* When the return value of strtol overflows the int type, don't overflow
	 * and return an overflow error code. */
	if (num > UINT_MAX) {
		num = UINT_MAX;
		err = -ERANGE;
	} else if (num < 0) {
		num = 0;
		err = -ERANGE;
	}
	*value = (unsigned int)num;

	return err;
}

int str_to_uint(const char *str, unsigned int *value)
{
	char *endptr;
	int err = str_to_uint_endptr(str, &endptr, value);

	if (err) {
		return err;
	}

	if (*endptr != '\0' && !isspace(*endptr)) {
		return -EINVAL;
	}

	return 0;
}

char *str_to_lower(const char *str)
{
	char *str_lower = malloc_or_die(str_len(str) + 1);

	for (int i = 0; str[i]; i++) {
		str_lower[i] = tolower(str[i]);
	}
	str_lower[str_len(str)] = '\0';

	return str_lower;
}

bool str_to_bool(const char *str)
{
	char *str_lower;
	bool ret = false;

	str_lower = str_to_lower(str);
	if (str_cmp(str_lower, "true") == 0) {
		ret = true;
	}

	/* return false for everything else */
	FREE(str_lower);
	return ret;
}

char *str_subchar(const char *str, char c1, char c2)
{
	char *new = strdup_or_die(str);
	for (int i = 0; new[i]; i++) {
		if (new[i] == c1) {
			new[i] = c2;
		}
	}

	return new;
}
