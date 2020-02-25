#ifndef __LIST__
#define __LIST__

/**
 * @file
 * @brief Doubly linked list implementation.
 *
 * All list functions that performs operation in all nodes should always require
 * that any element of the list is used as a parameter and return the first
 * element of the list. Caller is not required to use list_head() in the
 * parameter or result.
 *
 * List functions that operates in a single element, like list_append(),
 * list_prepend() have documentation on what's expected from caller.
 *
 * A NULL pointer is always considered an empty list, so all functions should
 * work when NULL is used as a list parameter.
 */

#include <stdbool.h>
#include <stdlib.h>

#include "comp_functions.h"

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

/** @brief Callback to free data used in list. */
typedef void (*list_free_data_fn_t)(void *data);

/** @brief Callback to filter a data in a list. */
typedef bool (*filter_fn_t)(const void *a);

/** @brief Callback to clone a data in a list. */
typedef void *(*clone_fn_t)(const void *a);

/**
 * @brief Append an element at the end of the list
 * @param list any element of the list
 * @param data the data the new element should keep
 *
 * @returns the appended element or NULL on failures.
 *
 * @note Created link can be used as the list parameter to efficiently append
 * new elements without having to traverse the whole list to find the last one.
 */
struct list *list_append_data(struct list *list, void *data);

/**
 * @brief Prepend one element to the list.
 * @param list any element of the list
 * @param data the data the new element should keep
 *
 * @returns the prepended elemement or NULL on failures.
 *
 * @note Created link can be used as the list parameter to efficiently prepend
 * new elements without having to traverse the whole list to find the last one.
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
int list_len(struct list *list);

/**
 * @brief Sorts the list using the comparison function.
 *
 * List can be any item in the list, the complete list will still be sorted.
 * Returns the first item in the sorted list.
 */
struct list *list_sort(struct list *list, comparison_fn_t comparison_fn);

/**
 * @brief Appends list2 at the tail of list1
 *
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
 *
 * Same as calling list_clone(list, NULL);
 */
struct list *list_clone(struct list *list);

/**
 * @brief Produce a cloned copy of list, cloning all elements using clone_fn
 *
 * If clone_fn is NULL, elements aren't going to be cloned.
 */
struct list *list_clone_deep(struct list *list, clone_fn_t clone_fn);

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
 * @brief removes duplicated elements from a sorted list
 */
struct list *list_sorted_deduplicate(struct list *list, comparison_fn_t comparison_fn, list_free_data_fn_t list_free_data_fn);

/**
 * @brief Splits any element from sorted list list1 that happens to be in the sorted list list2 meeting the criteria and moves it to common_elements.
 */
struct list *list_sorted_split_common_elements(struct list *list1, struct list *list2, struct list **common_elements, comparison_fn_t comparison_fn, list_free_data_fn_t list_free_data_fn);

/**
 * @brief Filters any element from sorted list list1 that happens to be in the sorted list list2 meeting the criteria.
 */
struct list *list_sorted_filter_common_elements(struct list *list1, struct list *list2, comparison_fn_t comparison_fn, list_free_data_fn_t list_free_data_fn);

/**
 * @brief removes the first occurrence of an item and returns a pointer to its data.
 */
void *list_remove(void *item_to_remove, struct list **list, comparison_fn_t comparison_fn);

/**
 * @brief moves all occurrences of the specified item if found from list1 to list2
 */
void list_move_item(void *item_to_move, struct list **list1, struct list **list2, comparison_fn_t comparison_fn);

/**
 * @brief Filters from list any element that doesn't fit the condition defined in filter_fn.
 */
struct list *list_filter_elements(struct list *list, filter_fn_t filter_fn, list_free_data_fn_t list_free_data_fn);

#endif
