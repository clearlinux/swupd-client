#ifndef __INCLUDE_GUARD_HASH_H
#define __INCLUDE_GUARD_HASH_H

/**
 * @file
 * @brief Hashmap implementation.
 */

#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Get the size of a hashmap from the number of used bits. */
#define HASH_SIZE(bits) (1 << bits)

/** @brief Callback to free data from this hashmap. */
typedef void (*free_data_fn_t)(void *data);

/** @brief Callback to get the hash from one specific data. */
typedef size_t (*hash_fn_t)(const void *data);

/** @brief Callback to compare two different user's data. */
typedef bool (*hash_equal_fn_t)(const void *a, const void *b);

/** @brief Hashmap internal structure. */
struct hashmap {
	/** @brief Bits used in hashmap mask. */
	unsigned int mask_bits;
	/** @brief Save callback to get the hash from one specific data. */
	hash_equal_fn_t equal;
	/** @brief Save the callback to get the hash from one specific data. */
	hash_fn_t hash;
	/** @brief Internal map structure */
	struct list *map[];
};

/**
 * @brief Create a new hashmap.
 *
 * Size of the hashmap will be the closest power of 2 number greater than capacity.
 * Function hash will be used as a hash function and equal to compare elements.
 */
struct hashmap *hashmap_new(size_t capacity, hash_equal_fn_t equal, hash_fn_t hash);

/**
 * @brief Put an element into the hashmap, if it isn't already in the hashmap.
 *
 * @returns true if the element was added or false if the element was already
 * in the hashmap.
 */
bool hashmap_put(struct hashmap *hashmap, void *data);

/**
 * @brief Retrieve an element from the hashmap.
 *
 * @returns the element or NULL if not found.
 */
void *hashmap_get(struct hashmap *hashmap, const void *key);

/**
 * @brief Remove an element from the hashmap.
 *
 * @returns the removed element or NULL if not found.
 */
void *hashmap_pop(struct hashmap *hashmap, const void *key);

/**
 * @brief Check if the hashmap contains a specific element.
 */
bool hashmap_contains(struct hashmap *hashmap, const void *key);

/**
 * @brief Free the hashmap.
 */
void hashmap_free(struct hashmap *hashmap);

/**
 * @brief Free the hashmap and all data using free_data() function.
 */
void hashmap_free_hash_and_data(struct hashmap *hashmap, free_data_fn_t free_data);

/**
 * @brief Hash function helper to calculate a good hash to be used with strings.
 */
size_t hashmap_hash_from_string(const char *key);

/**
 * @brief Print a hashmap, for debug purposes.
 */
void hashmap_print(struct hashmap *hashmap, void(print_data)(void *data));

/**
 * @brief Loop through all elements in the hashmap.
 *
 * @param hashmap The hashmap to iterate.
 * @param i will be set with the current position on the hash table.
 * @param l will be set with the current list element.
 * @param out_data will be set with the data of the current element.
 */
#define HASHMAP_FOREACH(hashmap, i, l, out_data)                         \
	for (i = 0; i < HASH_SIZE(hashmap->mask_bits); i++)              \
		for (l = hashmap->map[i], out_data = l ? l->data : NULL; \
		     l && (out_data = l->data); l = l->next)

#ifdef __cplusplus
}
#endif

#endif
