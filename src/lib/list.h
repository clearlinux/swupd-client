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
 * The comparison function should return 0 if a is equal to b,
 * any number "< 0" if a is lower than b and any number "> 0" if
 * a is bigger than b.
 * This behavior is similar to strcmp() and other standard compare functions.
 */
typedef int (*comparison_fn_t)(const void *a, const void *b);

/** @brief Callback to free data used in list. */
typedef void (*list_free_data_fn_t)(void *data);

/** @brief Callback to filter a data in a list. */
typedef bool (*filter_fn_t)(const void *a);

/** @brief Callback to cone a data in a list. */
typedef void *(*clone_fn_t)(const void *a);

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
int list_len(struct list *list);

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
 * @brief strcmp to be used with list_search() or list_sort() on a list of strings.
 */
int list_strcmp(const void *a, const void *b);

/**
 * @brief removes duplicated elements from a list
 *
 * Function requirements:
 * - the list has to be sorted
 * - the comparison_fn should behave similar to strcmp(), returning 0 if it's a match, and "< 0" or "> 0" if it's not
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

/**
 * @brief removes the first occurrence of an item and returs a pointer to its data.
 *
 * Function requirements:
 * - the comparison_fn should behave similar to strcmp(), returning 0 if it's a match, and "< 0" or "> 0" if it's not
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
