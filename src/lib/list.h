#ifndef __LIST__
#define __LIST__

/**
 * @file
 * @brief Doubly linked list implementation.
 */

#include <stdbool.h>
#include <stdlib.h>

/**
 * @brief List node.
 */
struct list {
	/** @brief Node's data */
	void *data;
	/** @brief Previous list node. */
	struct list *prev;
	/** @brief Next list node. */
	struct list *next;
};

/**
 * @brief Callback to compare two datas in a list.
 * The comparison_fn should behave similar to strcmp(), returning 0 if
 * it is a match, and "< 0" or "> 0" if it is not.
 */
typedef int (*comparison_fn_t)(const void *a, const void *b);

/** @brief Callback to free data used in list. */
typedef void (*list_free_data_fn_t)(void *data);

/**
 * Creates a new list item, store data, and inserts item in list (which can
 * be NULL). Returns created link, or NULL if failure. Created link can be
 * used as the list parameter to efficiently append new elements without
 * having to traverse the whole list to find the last one.
 */
struct list *list_append_data(struct list *list, void *data);

/**
 * @brief Prepend one element to the list.
 */
struct list *list_prepend_data(struct list *list, void *data);

/**
 * @brief Returns the head of a list given anyone of its items.
 */
struct list *list_head(struct list *item);

/**
 * @brief Returns the tail of a list given anyone of its items.
 */
struct list *list_tail(struct list *item);

/**
 * @brief Returns the length of a list given anyone of its items.
 */
unsigned int list_len(struct list *list);

/**
 * @brief Sorts the list using the comparison function.
 *
 * List can be any item in the list, the complete list will still be sorted.
 * Returns the first item in the sorted list.
 */
struct list *list_sort(struct list *list, comparison_fn_t comparison_fn);

/**
 * @brief Appends list2 at the tail of list1. Either list1 or list2 can be NULL.
 * @returns The head of the resulting concatenation
 */
struct list *list_concat(struct list *list1, struct list *list2);

/**
 * @brief Unlink item from its list, call list_free_data_fn to free its data,
 * and free the item itself. Always returns the preceding item, except in
 * case head is freed - then return the new head. If this was the last item,
 * returns NULL.
 */
struct list *list_free_item(struct list *item, list_free_data_fn_t list_free_data_fn);

/**
 * @brief Destroy all items in the list, calling list_free_data_fn for each
 * item to let user free its own item data. list_free_data_fn can be NULL.
 */
void list_free_list_and_data(struct list *list, list_free_data_fn_t list_free_data_fn);

/**
 * @brief Destroy all items in the list without disposing item data.
 */
void list_free_list(struct list *list);

/**
 * @brief Shallow copy of the the list.
 */
struct list *list_clone(struct list *list);

/**
 * @brief Deep copy of a string list
 */
struct list *list_deep_clone_strs(struct list *source);

/**
 * @brief Check list length.
 */
bool list_longer_than(struct list *list, int count);

/**
 * @brief Search 'item' on 'list' using 'comparisson_fn' function to compare elements.
 *
 * 'comparison_fn' is always called with one item from the 'list' as first
 * argument and 'item' as the second.
 * If one element is found it is returned. NULL is returned otherwise.
 *
 * @note You can use list_strcmp to search a string in a string list:
 *       list_search(list, "my_string", list_strcmp);
 */
void *list_search(struct list *list, const void *item, comparison_fn_t comparison_fn);

/**
 * @brief strcmp to be used with list_search() or list_sort() on a list of strings.
 */
int list_strcmp(const void *a, const void *b);

/**
 * @brief removes duplicated elements from a list
 *
 */
struct list *list_deduplicate(struct list *list, comparison_fn_t comparison_fn, list_free_data_fn_t list_free_data_fn);

/**
 * @brief Filters any element from list1 that happens to be in list2 as long as it meets the criteria
 *
 * Function requirements:
 * - list1 and list2 are sorted
 * - the comparison_fn should behave similar to strcmp(), returning 0 if it's a match, and "< 0" or "> 0" if it's not
 */
struct list *list_filter_common_elements(struct list *list1, struct list *list2, comparison_fn_t comparison_fn, list_free_data_fn_t list_free_data_fn);

#endif
