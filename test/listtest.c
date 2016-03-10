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
 *   Authors:
 *         pplaquet <paul.plaquette@intel.com>
 *         Eric Lapuyade <eric.lapuyade@intel.com>
 *
 */

/* list test */

#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "list.h"
#include "swupd.h"

static int data_compare(const void *a, const void *b)
{
	unsigned int *A, *B;

	A = (unsigned int *)a;
	B = (unsigned int *)b;
	// printf("comparing data %d > %d\n", *A, *B);
	return (*A - *B);
}

static int data_compare_reverse(const void *a, const void *b)
{
	unsigned int *A, *B;

	A = (unsigned int *)a;
	B = (unsigned int *)b;
	// printf("comparing data %d > %d\n", *A, *B);
	return (*B - *A);
}

#if 0
static void dump_list(struct list *list)
{
	int i;
	list = list_head(list);

	i = 1;
	while (list) {
		printf("Item %d %lx, data = %d\n, next = %p, prev = %p\n", i,
			(long unsigned int) list, *((unsigned int*)(list->data)),
			list->next, list->prev);
		list = list->next;
		i++;
	}
}
#endif

static int check_list_order(struct list *list, int sort_criteria)
{
	struct list *item;

	item = list_head(list);
	while (item && item->next) {
		if (sort_criteria > 0) {
			if (data_compare(item->data, item->next->data) > 0) {
				return EXIT_FAILURE;
			}
		} else {
			if (data_compare(item->data, item->next->data) < 0) {
				return EXIT_FAILURE;
			}
		}
		item = item->next;
	}

	return 0;
}

#define TEST_LIST_LEN 20

int main(int argc, char **argv)
{
	struct list *list = NULL;
	struct list *list1, *list2;
	struct list *item, *item2, *item3, *head, *tail;
	unsigned int i;
	struct timeval tod;
	unsigned int seed;
	unsigned int len = TEST_LIST_LEN;
	clock_t t;
	unsigned int *data;

	/* List length must be greater than 3 for all tests to work */
	if (argc > 1) {
		len = atoi(argv[1]);
	}
	if (len < 4) {
		printf("List length must be at least 4 for tests.\n");
		return EXIT_FAILURE;
	}

	/* seed the random generator so that we get different lists each time */
	gettimeofday(&tod, NULL);
	seed = (unsigned int)tod.tv_sec;
	srand(seed);

	/* create a list with random data between 0 and len */
	for (i = 1; i <= len; i++) {
		data = malloc(sizeof(unsigned int));
		if (!data) {
			printf("data allocation failed\n");
			exit(-1);
		}
		*data = (unsigned int)rand() % len;
		list = list_append_data(list, data);
	}
	printf("List constructed, seed = %d, len = %d\n", seed, list_len(list));

	t = clock();
	list = list_sort(list, data_compare);
	t = clock() - t;

	/* check list elements are in right order */
	if (check_list_order(list, 1) != 0) {
		printf("Sorted (1) List is in wrong order\n");
		return EXIT_FAILURE;
	}

	/* check sorted list has the expected len */
	if (list_len(list) != len) {
		printf("Wrong sorted (1) list len = %d instead of %d", i, len);
		return EXIT_FAILURE;
	}
	// dump_list(list);
	printf("List sorted in %f seconds\n", (float)t / CLOCKS_PER_SEC);

	/* sort again on sorted list to check special case */

	t = clock();
	list = list_sort(list, data_compare);
	t = clock() - t;

	if (check_list_order(list, 1) != 0) {
		printf("Sorted (2) List is in wrong order\n");
		return EXIT_FAILURE;
	}
	if (list_len(list) != len) {
		printf("Wrong sorted (2) list len = %d instead of %d", i, len);
		return EXIT_FAILURE;
	}
	// dump_list(list);
	printf("Sorted list sorted again in %f seconds\n", (float)t / CLOCKS_PER_SEC);

	/* reverse sort from sorted state */

	t = clock();
	list = list_sort(list, data_compare_reverse);
	t = clock() - t;

	if (check_list_order(list, -1) != 0) {
		printf("Sorted (3) List is in wrong order\n");
		return EXIT_FAILURE;
	}
	if (list_len(list) != len) {
		printf("Wrong sorted (3) list len = %d instead of %d", i, len);
		return EXIT_FAILURE;
	}
	// dump_list(list);
	printf("Sorted list sorted reverse in %f seconds\n", (float)t / CLOCKS_PER_SEC);

	/* Check freeing the head item.
	 * This must return the 2nd item, which must be the new head */
	head = list_head(list);
	item2 = head->next;
	list = list_free_item(head, free);
	if (list != item2) {
		printf("removing head item did not return 2nd item\n");
		return EXIT_FAILURE;
	}
	if (item2->prev) {
		printf("item returned after removing head is not new head\n");
		return EXIT_FAILURE;
	}
	if (list_len(item2) != len - 1) {
		printf("removing head item did not result in the right list len\n");
		return EXIT_FAILURE;
	}
	printf("Removing head correctly returned 2nd item as new head\n");

	/* Check freeing middle item, must return previous item */
	head = list_head(list);
	item2 = head->next;
	item3 = item2->next;
	list = list_free_item(item2, free);
	if (list != head) {
		printf("removing 2nd item did not return head item\n");
		return EXIT_FAILURE;
	}
	if ((head != item3->prev) || (head->next != item3)) {
		printf("removing 2nd item did not link 3rd item to head\n");
		return EXIT_FAILURE;
	}

	if (list_len(list) != len - 2) {
		printf("removing 2nd item did not result in the right list len\n");
		return EXIT_FAILURE;
	}
	printf("Removing middle item correctly returned previous item\n");

	/* Check freeing tail, must return new tail */
	tail = list_tail(list);
	item = tail->prev;
	list = list_free_item(tail, free);
	if (list != item) {
		printf("removing tail did not return prev item\n");
		return EXIT_FAILURE;
	}
	tail = list_tail(list);
	if (list != tail) {
		printf("removing tail did not return new tail\n");
		return EXIT_FAILURE;
	}
	if (list_len(list) != len - 3) {
		printf("removing tail did not result in the right list len\n");
		return EXIT_FAILURE;
	}
	printf("Removing tail correctly returned previous item as new tail\n");

	list_free_list_and_data(list, free);
	list = NULL;

	/* Check list_concat */
	list1 = NULL;
	list2 = NULL;

	for (i = 3; i > 0; i--) {
		data = malloc(sizeof(unsigned int));
		if (!data) {
			printf("data allocation failed\n");
			exit(-1);
		}
		*data = (unsigned int)i;
		list1 = list_prepend_data(list1, data);
	}

	for (i = 6; i > 3; i--) {
		data = malloc(sizeof(unsigned int));
		if (!data) {
			printf("data allocation failed\n");
			exit(-1);
		}
		*data = (unsigned int)i;
		list2 = list_prepend_data(list2, data);
	}

	/* Check concat one list with empty list */
	list = list_concat(list1, NULL);
	if (list_len(list) != 3) {
		printf("concat(list1, NULL) did not result in a list len of 3\n");
		return EXIT_FAILURE;
	}
	if (list != list1) {
		printf("concat(list1, NULL) did not return list1\n");
		return EXIT_FAILURE;
	}
	if (list_head(list) != list) {
		printf("concat(list1, NULL) did not return list1 head\n");
		return EXIT_FAILURE;
	}
	if (*((unsigned int *)(list->data)) != 1) {
		printf("concat(list1, NULL) head is wrong\n");
		return EXIT_FAILURE;
	}
	printf("concat(list1, NULL) is OK\n");
	// dump_list(list);

	/* Check concat empty list with one list*/
	list = list_concat(NULL, list2);
	if (list_len(list) != 3) {
		printf("concat(NULL, list2) did not result in a list len of 3\n");
		return EXIT_FAILURE;
	}
	if (list != list2) {
		printf("concat(NULL, list2) did not return list2\n");
		return EXIT_FAILURE;
	}
	if (list_head(list) != list) {
		printf("concat(NULL, list2) did not return list2 head\n");
		return EXIT_FAILURE;
	}
	if (*((unsigned int *)(list->data)) != 4) {
		printf("concat(NULL, list2) head is wrong\n");
		return EXIT_FAILURE;
	}
	printf("concat(NULL, list2) is OK\n");
	// dump_list(list);

	/* Check concat two lists */
	list = list_concat(list1->next, list2->next->next);
	if (list_len(list) != 6) {
		printf("concat(list1, list2) did not result in a list len of 6\n");
		return EXIT_FAILURE;
	}
	if (*((unsigned int *)(list->data)) != 1) {
		printf("concat(list1, list2) did not return list1 head\n");
		return EXIT_FAILURE;
	}
	if (*((unsigned int *)(list->next->next->next->data)) != 4) {
		printf("concat(list1, list2) 4th item is not 4\n");
		return EXIT_FAILURE;
	}
	printf("concat(list1, list2) is OK\n");
	// dump_list(list);

	list_free_list_and_data(list, free);

	printf("*** ALL LIST TESTS COMPLETED OK***\n");

	return EXIT_SUCCESS;
}
