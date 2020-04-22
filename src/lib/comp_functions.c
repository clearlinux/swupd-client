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

#include <string.h>

#include "comp_functions.h"
#include "strings.h"

/**
 * All comparison functions in this file should match this type definition:
 * typedef int (*comparison_fn_t)(const void *a, const void *b);
 *
 * They should behave like this (similar to str_cmp):
 * - return 0 if a is equal to b
 * - return any number "< 0" if a is lower than b
 * - return any number "> 0" if a is bigger than b
 */

int str_cmp_wrapper(const void *a, const void *b)
{
	return str_cmp((const char *)a, (const char *)b);
}
