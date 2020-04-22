#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "../../src/lib/strings.h"
#include "../../src/lib/list.h"
#include "test_helper.h"

void test_list_sorted_deduplicate()
{
	struct list *list = NULL;
	char *str;

	// Deduplicating an empty list
	list = list_sorted_deduplicate(NULL, strcmp_wrapper, NULL);
	check(list == NULL);

	// Deduplicating a list of a single element
	list = list_prepend_data(list, "A");
	list = list_sorted_deduplicate(list, strcmp_wrapper, NULL);

	str = str_join(", ", list);
	check(strcmp("A", str) == 0);
	free(str);

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
	list = list_sorted_deduplicate(list, strcmp_wrapper, NULL);

	str = str_join(", ", list);
	check(strcmp("A, B, C, D, E", str) == 0);
	free(str);
	list_free_list(list);

}

static bool filterX(const void *data)
{
	return strcmp(data, "X") != 0;
}

void test_list_filter_elements()
{
	struct list *list = NULL;
	char *str;

	// Filter empty list
	list = list_sorted_deduplicate(NULL, strcmp_wrapper, NULL);
	check(list == NULL);

	// Don't remove element
	list = list_prepend_data(list, "A");
	list = list_filter_elements(list, filterX, NULL);

	str = str_join(", ", list);
	check(strcmp("A", str) == 0);
	free(str);
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
	check(strcmp("A, B, C, D, E, F", str) == 0);
	free(str);
	list_free_list(list);
}

int main() {
	test_list_sorted_deduplicate();
	test_list_filter_elements();

	return 0;
}
