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

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "list.h"

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
	if (item) {
		item->data = data;
		item->next = NULL;
		item->prev = NULL;
	}

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
	struct list *right;
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
	struct list *item = NULL;

	item = list_alloc_item(data);
	if (item) {
		item = list_append_item(list, item);
	}

	return item;
}

struct list *list_prepend_data(struct list *list, void *data)
{
	struct list *item = NULL;

	item = list_alloc_item(data);
	if (item) {
		item = list_prepend_item(list, item);
	}

	return item;
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

unsigned int list_len(struct list *list)
{
	unsigned int len;
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

#if 0
struct list *list_find_data(struct list *list, void *data)
{
	list = list_head(list);
	while (list) {
		if (list->data == data) {
			return list;
		}
		list = list->next;
	}

	return NULL;
}
#endif

struct list *list_sort(struct list *list, comparison_fn_t comparison_fn)
{
	list = list_head(list);
	unsigned int len = list_len(list);
	return list_merge_sort(list, len, comparison_fn);
}

struct list *list_concat(struct list *list1, struct list *list2)
{
	struct list *tail;

	list2 = list_head(list2);

	if (list1 == NULL) {
		return list2;
	}

	list1 = list_head(list1);

	if (list2) {
		tail = list_tail(list1);

		tail->next = list2;
		list2->prev = tail;
	}

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

	free(item);

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
