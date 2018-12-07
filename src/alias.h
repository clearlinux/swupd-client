#ifndef __ALIAS__
#define __ALIAS__

#include "lib/list.h"

struct alias_lookup {
	char *alias;
	struct list *bundles;
};

void free_alias_lookup(void *lookup);
struct list *get_alias_bundles(struct list *alias_definitions, char *alias);
struct list *get_alias_definitions(void);
#endif /* __ALIAS__ */
