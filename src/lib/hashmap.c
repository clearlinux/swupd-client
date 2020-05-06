/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2020 Intel Corporation.
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
 *         Otavio Pontes <otavio.pontes@intel.com>
 *
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashmap.h"
#include "list.h"
#include "log.h"
#include "macros.h"

#define HASH_MASK(bits) (HASH_SIZE(bits) - 1)

size_t hashmap_hash_from_string(const char *key)
{
	size_t hash = 0;

	while (*key) {
		hash = hash * 29 /* a prime number */ + (unsigned char)*key;
		key++;
	}

	return hash;
}

static inline struct list **get_hashmap_list(struct hashmap *hashmap, const void *data)
{
	int mask = HASH_MASK(hashmap->mask_bits);
	return &hashmap->map[hashmap->hash(data) & (size_t)mask];
}

static unsigned int calc_bits(size_t capacity)
{
	unsigned int bits = 0;

	if (!capacity) {
		return 0;
	}

	capacity--;
	while (capacity) {
		capacity >>= 1;
		bits++;
	}

	return bits;
}

struct hashmap *hashmap_new(size_t capacity, hash_equal_fn_t equal, hash_fn_t hash)
{
	struct hashmap *hashmap;
	unsigned int mask_bits = calc_bits(capacity);
	size_t real_capacity = (size_t)HASH_SIZE(mask_bits);

	hashmap = malloc_or_die(sizeof(struct hashmap) + real_capacity * sizeof(struct list *));
	hashmap->mask_bits = mask_bits;
	hashmap->hash = hash;
	hashmap->equal = equal;

	return hashmap;
}

void hashmap_free_hash_and_data(struct hashmap *hashmap, free_data_fn_t free_data)
{
	int i;

	for (i = 0; i < HASH_SIZE(hashmap->mask_bits); i++) {
		list_free_list_and_data(hashmap->map[i], free_data);
	}

	FREE(hashmap);
}

void hashmap_free(struct hashmap *hashmap)
{
	hashmap_free_hash_and_data(hashmap, NULL);
}

bool hashmap_put(struct hashmap *hashmap, void *data)
{
	struct list **items, *i;

	items = get_hashmap_list(hashmap, data);
	for (i = *items; i; i = i->next) {
		if (hashmap->equal(data, i->data)) {
			return false;
		}
	}
	*items = list_prepend_data(*items, data);

	return true;
}

static struct list *hashmap_get_internal(struct hashmap *hashmap, const void *key, bool remove)
{
	struct list **items, *i;

	items = get_hashmap_list(hashmap, key);
	for (i = *items; i; i = i->next) {
		if (hashmap->equal(key, i->data)) {
			void *data = i->data;
			if (remove) {
				//If it's the first element of the list, update head
				if (i == *items) {
					*items = i->next;
				}
				list_free_item(i, NULL);
			}
			return data;
		}
	}

	return NULL;
}

void *hashmap_get(struct hashmap *hashmap, const void *key)
{
	return hashmap_get_internal(hashmap, key, false);
}

void *hashmap_pop(struct hashmap *hashmap, const void *key)
{
	return hashmap_get_internal(hashmap, key, true);
}

bool hashmap_contains(struct hashmap *hashmap, const void *key)
{
	return hashmap_get_internal(hashmap, key, false) != NULL;
}

void hashmap_print(struct hashmap *hashmap, void(print_data)(void *data))
{
	struct list *l;
	void *out_data;
	int i;
	size_t count = 0;

	if (!hashmap) {
		return;
	}

	info("Hashmap (bits: %x, size: %d)\n", hashmap->mask_bits,
	     HASH_SIZE(hashmap->mask_bits));

	for (i = 0; i < HASH_SIZE(hashmap->mask_bits); i++) {
		info("map[%d] list(%d) {", i, list_len(hashmap->map[i]));
		for (l = hashmap->map[i], out_data = l ? l->data : NULL;
		     l && (out_data = l->data); l = l->next) {
			if (print_data) {
				print_data(out_data);
			}
			count++;
		}
		info("}\n");
	}

	info("Total elements: %ld\n", count);

	return;
}
