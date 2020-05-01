/*
 *   Software Updater - client side
 *
 *      Copyright (c) 2019-2020 Intel Corporation.
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
 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "../../src/lib/strings.h"
#include "../../src/lib/list.h"
#include "../../src/lib/macros.h"
#include "test_helper.h"

void test_list_sorted_deduplicate()
{
	struct list *list = NULL;
	char *str;

	// Deduplicating an empty list
	list = list_sorted_deduplicate(NULL, str_cmp_wrapper, NULL);
	check(list == NULL);

	// Deduplicating a list of a single element
	list = list_prepend_data(list, "A");
	list = list_sorted_deduplicate(list, str_cmp_wrapper, NULL);

	str = str_join(", ", list);
	check(str_cmp("A", str) == 0);
	FREE(str);

	// Deduplicating a list of odd number of identical elements
	list = list_append_data(list, "A");
	list = list_append_data(list, "A");
	list = list_append_data(list, "B");
	list = list_append_data(list, "B");
	list = list_append_data(list, "C");
	list = list_append_data(list, "D");
	list = list_append_data(list, "D");
	list = list_append_data(list, "D");
	list = list_append_data(list, "D");
	list = list_append_data(list, "E");
	list = list_append_data(list, "E");
	list = list_append_data(list, "E");
	list = list_head(list);
	list = list_sorted_deduplicate(list, str_cmp_wrapper, NULL);

	str = str_join(", ", list);
	check(str_cmp("A, B, C, D, E", str) == 0);
	FREE(str);
	list_free_list(list);

}

static bool filterX(const void *data)
{
	return str_cmp(data, "X") != 0;
}

void test_list_filter_elements()
{
	struct list *list = NULL;
	char *str;

	// Filter empty list
	list = list_sorted_deduplicate(NULL, str_cmp_wrapper, NULL);
	check(list == NULL);

	// Don't remove element
	list = list_prepend_data(list, "A");
	list = list_filter_elements(list, filterX, NULL);

	str = str_join(", ", list);
	check(str_cmp("A", str) == 0);
	FREE(str);
	list_free_list(list);
	list = NULL;

	// Remove single element
	list = list_prepend_data(list, "X");
	list = list_filter_elements(list, filterX, NULL);
	check(list == NULL);

	// Remove all elements
	list = list_prepend_data(list, "X");
	list = list_prepend_data(list, "X");
	list = list_prepend_data(list, "X");
	list = list_prepend_data(list, "X");
	list = list_prepend_data(list, "X");
	list = list_prepend_data(list, "X");
	list = list_prepend_data(list, "X");
	list = list_prepend_data(list, "X");
	list = list_prepend_data(list, "X");
	list = list_prepend_data(list, "X");
	list = list_filter_elements(list, filterX, NULL);
	check(list == NULL);

	// Remove some elements
	list = list_append_data(list, "X");
	list = list_append_data(list, "X");
	list = list_append_data(list, "X");
	list = list_append_data(list, "A");
	list = list_append_data(list, "B");
	list = list_append_data(list, "C");
	list = list_append_data(list, "X");
	list = list_append_data(list, "D");
	list = list_append_data(list, "X");
	list = list_append_data(list, "X");
	list = list_append_data(list, "X");
	list = list_append_data(list, "E");
	list = list_append_data(list, "X");
	list = list_append_data(list, "X");
	list = list_append_data(list, "F");
	list = list_append_data(list, "X");
	list = list_head(list);
	list = list_filter_elements(list, filterX, NULL);

	str = str_join(", ", list);
	check(str_cmp("A, B, C, D, E, F", str) == 0);
	FREE(str);
	list_free_list(list);
}

int main() {
	test_list_sorted_deduplicate();
	test_list_filter_elements();

	return 0;
}
