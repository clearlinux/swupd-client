#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "../../src/lib/strings.h"
#include "../../src/lib/list.h"
#include "test_helper.h"

void test_list_deduplicate()
{
	struct list *list = NULL;
	char *str;

	// Deduplicating an empty list
	list = list_deduplicate(NULL, list_strcmp, NULL);
	check(list == NULL);

	// Deduplicating a list of a single element
	list = list_prepend_data(list, "A");
	list = list_deduplicate(list, list_strcmp, NULL);

	str = string_join(", ", list);
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
	list = list_deduplicate(list, list_strcmp, NULL);

	str = string_join(", ", list);
	check(strcmp("A, B, C, D, E", str) == 0);
	free(str);
	list_free_list(list);

}

int main() {
	test_list_deduplicate();

	return 0;
}
