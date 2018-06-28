#ifndef __INCLUDE_GUARD_HASH_H
#define __INCLUDE_GUARD_HASH_H

#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HASH_SIZE(bits) (1 << bits)

typedef void (*free_data_fn_t)(void *data);
typedef size_t (*hash_fn_t)(const void *data);
typedef bool (*hash_equal_fn_t)(const void *a, const void *b);

struct hashmap {
	unsigned int mask_bits;
	hash_equal_fn_t equal;
	hash_fn_t hash;
	struct list *map[];
};

/*
 * Create a new hashmap. Size of the hashmap will be the closest power of 2 number
 * greater than capacity.
 * Function hash will be used as a hash function and equal to compare elements.
 */
struct hashmap *hashmap_new(size_t capacity, hash_equal_fn_t equal, hash_fn_t hash);

/*
 * Put an element into the hashmap, if it isn't already in the hashmap.
 *
 * Return true if the element was added or false if the element was already
 * in the hashmap.
 */
bool hashmap_put(struct hashmap *hashmap, void *data);

/*
 * Retrieve an element from the hashmap.
 *
 * Return the element or NULL if not found.
 */
void *hashmap_get(struct hashmap *hashmap, const void *key);

/*
 * Remove an element from the hashmap.
 *
 * Return the removed element or NULL if not found.
 */
void *hashmap_pop(struct hashmap *hashmap, const void *key);

/*
 * Check if the hashmap contains a specific element.
 */
bool hashmap_contains(struct hashmap *hashmap, const void *key);

/*
 * Free the hashmap.
 */
void hashmap_free(struct hashmap *hashmap);

/*
 * Free the hashmap and all data using free_data() function.
 */
void hashmap_free_hash_and_data(struct hashmap *hashmap, free_data_fn_t free_data);

/*
 * Loop through all elements in the hashmap.
 *
 * i will be set with the current position on the hash table.
 * l will be set with the current list element.
 * data will be set with the data of the current element.
 */
#define HASHMAP_FOREACH(hashmap, i, l, out_data)                         \
	for (i = 0; i < HASH_SIZE(hashmap->mask_bits); i++)              \
		for (l = hashmap->map[i], out_data = l ? l->data : NULL; \
		     l && (out_data = l->data); l = l->next)

#ifdef __cplusplus
}
#endif

#endif
