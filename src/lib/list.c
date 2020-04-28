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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "list.h"
#include "macros.h"
#include "strings.h"

static struct list *list_append_item(struct list *list, struct list *item)
{
	list = list_tail(list);
	if (list) {
		list->next = item;
		item->prev = list;
	}

	return item;
}

static struct list *list_prepend_item(struct list *list, struct list *item)
{
	list = list_head(list);
	if (list) {
		list->prev = item;
		item->next = list;
	}

	return item;
}

static struct list *list_alloc_item(void *data)
{
	struct list *item;

	item = (struct list *)malloc(sizeof(struct list));
	ON_NULL_ABORT(item);

	item->data = data;
	item->next = NULL;
	item->prev = NULL;

	return item;
}

// Merges two sorted lists
static struct list *list_merge(struct list *list1, struct list *list2, comparison_fn_t comparison_fn)
{
	struct list *merged_list = NULL;
	struct list *merged_list_head = NULL;
	while (list1 && list2) {
		if (comparison_fn(list1->data, list2->data) < 0) {
			if (merged_list) {
				merged_list->next = list1;
				list1->prev = merged_list;
			}
			merged_list = list1;
			list1 = list1->next;
		} else {
			if (merged_list) {
				merged_list->next = list2;
				list2->prev = merged_list;
			}
			merged_list = list2;
			list2 = list2->next;
		}
		if (!merged_list_head) {
			merged_list_head = merged_list;
		}
	}
	if (list1 != NULL) {
		merged_list->next = list1;
		list1->prev = merged_list;
	}
	if (list2 != NULL) {
		merged_list->next = list2;
		list2->prev = merged_list;
	}
	return merged_list_head;
}

/* Splits a list into two halves to be merged */
static struct list *list_merge_sort(struct list *left, unsigned int len, comparison_fn_t comparison_fn)
{
	struct list *right = NULL;
	unsigned int left_len = len / 2;
	unsigned int right_len = len / 2 + len % 2;
	unsigned int i;

	if (!left) {
		return NULL;
	}

	if (len == 1) {
		return left;
	}

	right = left;

	/* Split into left and right lists */
	for (i = 0; i < left_len; i++) {
		right = right->next;
	}
	right->prev->next = NULL;
	right->prev = NULL;

	/* Recurse */
	left = list_merge_sort(left, left_len, comparison_fn);
	right = list_merge_sort(right, right_len, comparison_fn);
	return list_merge(left, right, comparison_fn);
}

/* ------------ Public API ------------ */

struct list *list_append_data(struct list *list, void *data)
{
	return list_append_item(list, list_alloc_item(data));
}

struct list *list_prepend_data(struct list *list, void *data)
{
	return list_prepend_item(list, list_alloc_item(data));
}

struct list *list_head(struct list *item)
{
	if (item) {
		while (item->prev) {
			item = item->prev;
		}
	}

	return item;
}

struct list *list_tail(struct list *item)
{
	if (item) {
		while (item->next) {
			item = item->next;
		}
	}

	return item;
}

int list_len(struct list *list)
{
	int len;
	struct list *item;

	if (list == NULL) {
		return 0;
	}

	len = 1;

	item = list;
	while ((item = item->next) != NULL) {
		len++;
	}

	item = list;
	while ((item = item->prev) != NULL) {
		len++;
	}

	return len;
}

struct list *list_sort(struct list *list, comparison_fn_t comparison_fn)
{
	list = list_head(list);
	int len = list_len(list);
	return list_merge_sort(list, len, comparison_fn);
}

bool list_is_sorted(struct list *list, comparison_fn_t comparison_fn)
{
	if (!comparison_fn) {
		return false;
	}

	for (list = list_head(list); list && list->next; list = list->next) {
		if (comparison_fn(list->data, list->next->data) > 0) {
			return false;
		}
	}

	return true;
}

struct list *list_concat(struct list *list1, struct list *list2)
{
	struct list *tail;

	list2 = list_head(list2);
	tail = list_tail(list1);
	list1 = list_head(list1);

	if (list1 == NULL) {
		return list2;
	}

	if (list2 == NULL) {
		return list1;
	}

	tail = list_tail(list1);
	tail->next = list2;
	list2->prev = tail;

	return list1;
}

struct list *list_free_item(struct list *item, list_free_data_fn_t list_free_data_fn)
{
	struct list *ret_item;

	if (item->prev) {
		item->prev->next = item->next;
		ret_item = item->prev;
	} else {
		ret_item = item->next;
	}

	if (item->next) {
		item->next->prev = item->prev;
	}

	if (list_free_data_fn) {
		list_free_data_fn(item->data);
	}

	FREE(item);

	return ret_item;
}

void list_free_list_and_data(struct list *list, list_free_data_fn_t list_free_data_fn)
{
	struct list *item = list_head(list);

	while (item) {
		item = list_free_item(item, list_free_data_fn);
	}
}

void list_free_list(struct list *list)
{
	list_free_list_and_data(list, NULL);
}

struct list *list_clone(struct list *list)
{
	return list_clone_deep(list, NULL);
}

/* deep clone a list of char * */
struct list *list_clone_deep(struct list *list, clone_fn_t clone_fn)
{
	struct list *clone = NULL;
	struct list *item;
	void *data;

	item = list_tail(list);
	while (item) {
		data = clone_fn ? clone_fn(item->data) : item->data;
		clone = list_prepend_data(clone, data);
		item = item->prev;
	}

	return clone;
}

bool list_longer_than(struct list *list, int count)
{
	struct list *item;

	item = list_head(list);
	while (item) {
		item = item->next;
		if (count-- < 0) {
			return true;
		}
	}
	return false;
}

void *list_search(struct list *list, const void *item, comparison_fn_t comparison_fn)
{
	struct list *heystack = list_head(list);

	for (; heystack; heystack = heystack->next) {
		if (comparison_fn(heystack->data, item) == 0) {
			return heystack->data;
		}
	}

	return NULL;
}

struct list *list_sorted_deduplicate(struct list *list, comparison_fn_t comparison_fn, list_free_data_fn_t list_free_data_fn)
{
	struct list *iter = NULL;
	void *item1, *item2 = NULL;

	iter = list = list_head(list);

	while (iter && iter->next) {

		item1 = iter->data;
		item2 = iter->next->data;

		if (comparison_fn(item1, item2) == 0) {
			list_free_item(iter->next, list_free_data_fn);
			/* an item was removed from the list, do not move to
			 * the next element, we need to re-check the same element
			 * against the new next item */
			continue;
		}
		iter = iter->next;
	}

	return list;
}

struct list *list_filter_elements(struct list *list, filter_fn_t filter_fn, list_free_data_fn_t list_free_data_fn)
{
	struct list *iter, *next;

	iter = list = list_head(list);
	while (iter) {
		next = iter->next;
		if (!filter_fn(iter->data)) {
			list_free_item(iter, list_free_data_fn);
			if (iter == list) {
				list = next;
			}
		}

		iter = next;
	}

	return list;
}

struct list *list_sorted_split_common_elements(struct list *list1, struct list *list2, struct list **common_elements, comparison_fn_t comparison_fn, list_free_data_fn_t list_free_data_fn)
{
	struct list *iter1, *iter2 = NULL;
	struct list *preserver = NULL;
	void *item1, *item2 = NULL;
	int comp;

	preserver = iter1 = list_head(list1);
	iter2 = list_head(list2);

	while (iter1 && iter2) {

		item1 = iter1->data;
		item2 = iter2->data;

		/* if the criteria has not been met, there is nothing to be done,
		 * just move the pointers to the next elements appropriately */
		comp = comparison_fn(item1, item2);
		if (comp < 0) {
			iter1 = iter1->next;
			continue;
		} else if (comp > 0) {
			iter2 = iter2->next;
			continue;
		}

		/* the condition was met */

		/* preserver was pointing to the first element of
		 * the list, if we are removing it, redirect it so it
		 * points to the new first element of the list */
		if (preserver == iter1) {
			preserver = iter1->next;
		}

		/* if a list was provided to store the common elements,
		 * store them, otherwise just remove the common element */
		if (common_elements) {
			*common_elements = list_prepend_data(*common_elements, item2);
		}

		iter1 = list_free_item(iter1, list_free_data_fn);
	}

	return preserver;
}

struct list *list_sorted_filter_common_elements(struct list *list1, struct list *list2, comparison_fn_t comparison_fn, list_free_data_fn_t list_free_data_fn)
{
	return list_sorted_split_common_elements(list1, list2, NULL, comparison_fn, list_free_data_fn);
}

void *list_remove(void *item_to_remove, struct list **list, comparison_fn_t comparison_fn)
{
	struct list *iter = NULL;
	void *item = NULL;
	int comp;

	*list = iter = list_head(*list);

	for (; iter; iter = iter->next) {

		item = iter->data;

		comp = comparison_fn(item, item_to_remove);
		if (comp == 0) {
			if (*list == iter) {
				/* "*list" should always point to the head of the list,
				 * if that is the element being removed, we need to
				 * point to the new head */
				*list = list_free_item(iter, NULL);
			} else {
				list_free_item(iter, NULL);
			}
			return item;
		}
	}

	/* item not found */
	return NULL;
}

void list_move_item(void *item_to_move, struct list **list1, struct list **list2, comparison_fn_t comparison_fn)
{
	void *item;
	//TODO: Improve eficience of this function. We are calling list_head()
	//      on list1 for all iterations on while()

	item = list_remove(item_to_move, list1, comparison_fn);
	while (item) {
		*list2 = list_prepend_data(*list2, item);
		item = list_remove(item_to_move, list1, comparison_fn);
	}
}
