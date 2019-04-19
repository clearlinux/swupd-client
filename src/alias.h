#ifndef __ALIAS__
#define __ALIAS__

/**
 * @file
 * @brief Bundle alias functions.
 */

#include "lib/list.h"

struct alias_lookup {
	char *alias;
	struct list *bundles;
};

/**
 * @brief Deep free on alias_lookup structs
 */
void free_alias_lookup(void *lookup);

/**
 * @brief Get bundles for a given alias but if the alias is
 * not actually an alias, return a list with just the
 * bundle name
 */
struct list *get_alias_bundles(struct list *alias_definitions, char *alias);

/**
 * @brief Parse an alias definition file for a list of bundles
 * it specifies
 *
 * Example file:
 * alias1	b1	b2
 * alias2	b3	b4
 *
 * If two files provide the same alias, the first one that
 * is parsed is the one that will be used.
 */
struct list *get_alias_definitions(void);

#endif /* __ALIAS__ */
