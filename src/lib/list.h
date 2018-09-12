#ifndef __LIST__
#define __LIST__

#include <stdlib.h>

/* Doubly linked list implementation for swupd.
 *
 * Note: All functions of this library will execute what is described starting
 * from the element that is pointed by list pointer and NULL is considered an
 * empty list.
 */

struct list {
	void *data;
	struct list *prev;
	struct list *next;
};

typedef int (*comparison_fn_t)(const void *a, const void *b);
typedef void (*list_free_data_fn_t)(void *data);
typedef void* (*list_clone_data_fn_t)(void *data);

/* Creates a new list item, store data and append it to the end of the list.
 *
 * Returns the a pointer to the list with the added element.
 * Note: This function is O(n), so avoid this. Prefer using list_insert_after()
 * Or list_prepend()
 */
struct list *list_append(struct list *list, void *data);

/* Creates a new list item, store data and insert it to the list just after
 * the node pointed by list.
 *
 * Returns A pointer to the new inserted element.
 */
struct list *list_insert_after(struct list *list, void *data);

/* Creates a new list item, store data and prepend it to the list just before
 * the node pointed by list.
 *
 * Returns a pointer to the list with the inserted element.
 */
struct list *list_prepend(struct list *list, void *data);

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

/* deep copy of a list, also cloning the elements */
struct list *list_deep_clone(struct list *list, list_clone_data_fn_t clone_fn);

/* Remove item from a list like list_free_item() and if item is the first
 * element of the list, update parameter list to point to the new first
 * element. */
void list_free_item_list(struct list **list, struct list *item, list_free_data_fn_t list_free_data_fn);

/* check list length */
extern int list_longer_than(struct list *list, int count);
#endif
