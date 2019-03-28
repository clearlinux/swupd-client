#ifndef __LIST__
#define __LIST__

#include <stdbool.h>
#include <stdlib.h>

struct list {
	void *data;
	struct list *prev;
	struct list *next;
};

typedef int (*comparison_fn_t)(const void *a, const void *b);
typedef void (*list_free_data_fn_t)(void *data);

/* creates a new list item, store data, and inserts item in list (which can
 * be NULL). Returns created link, or NULL if failure. Created link can be
 * used as the list parameter to efficiently append new elements without
 * having to traverse the whole list to find the last one. */
struct list *list_append_data(struct list *list, void *data);
struct list *list_prepend_data(struct list *list, void *data);

/* Returns the head or the tail of a list given anyone of its items */
struct list *list_head(struct list *item);
struct list *list_tail(struct list *item);

/* Returns the length of a list given anyone of its items */
unsigned int list_len(struct list *list);

/* Sorts the list using the comparison function. list can be any item in the
 * list, the complete list will still be sorted. Returns the first item in
 * the sorted list. */
struct list *list_sort(struct list *list, comparison_fn_t comparison_fn);

/* appends list2 at the tail of list1. Either list1 or list2 can be NULL.
 * returns the head of the resulting concatenation */
struct list *list_concat(struct list *list1, struct list *list2);

/* unlink item from its list, call list_free_data_fn to free its data,
 * and free the item itself. Always returns the preceding item, except in
 * case head is freed - then return the new head. If this was the last item,
 * returns NULL */
struct list *list_free_item(struct list *item, list_free_data_fn_t list_free_data_fn);
/* destroy all items in the list, calling list_free_data_fn for each
 * item to let user free its own item data. list_free_data_fn can be NULL */
void list_free_list_and_data(struct list *list, list_free_data_fn_t list_free_data_fn);
/* destroy all items in the list without disposing item data */
void list_free_list(struct list *list);

/* shallow copy of the the list */
struct list *list_clone(struct list *list);

/* deep copy of a string list */
struct list *list_deep_clone_strs(struct list *source);

/* check list length */
extern bool list_longer_than(struct list *list, int count);

/*
 * Search 'item' on 'list' using 'comparisson_fn' function to compare elements.
 * 'comparison_fn' is always called with one item from the 'list' as first
 * argument and 'item' as the second.
 * If one element is found it is returned. NULL is returned otherwise.
 *
 * Note: you can use list_strcmp to search a string in a string list:
 *       list_search(list, "my_string", list_strcmp);
 */
void *list_search(struct list *list, const void *item, comparison_fn_t comparison_fn);

/*
 * strcmp to be used with list_search() or list_sort() on a list of strings.
 */
int list_strcmp(const void *a, const void *b);

#endif
