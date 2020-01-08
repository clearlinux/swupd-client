/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2016 Intel Corporation.
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
 *         Arjan van de Ven <arjan@linux.intel.com>
 *         Tim Pepper <timothy.c.pepper@linux.intel.com>
 *
 */

#define _GNU_SOURCE
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "swupd.h"

struct list *subs;

static void free_subscription_data(void *data)
{
	struct sub *sub = (struct sub *)data;

	free_string(&sub->component);
	free(sub);
}

void free_subscriptions(struct list **subs)
{
	list_free_list_and_data(*subs, free_subscription_data);
	*subs = NULL;
}

struct list *free_list_file(struct list *item)
{
	return list_free_item(item, free_file_data);
}

int subscription_bundlename_strcmp(const void *a, const void *b)
{
	return strcmp(((struct sub *)a)->component, (const char *)b);
}

int subscription_sort_component(const void *a, const void *b)
{
	struct sub *A, *B;
	int ret;
	A = (struct sub *)a;
	B = (struct sub *)b;

	ret = strcmp(A->component, B->component);

	return ret;
}

/* Custom content comes from a different location and is not required to
 * have os-core added */
void read_subscriptions(struct list **subs)
{
	bool have_os_core = false;
	char *path = NULL;
	DIR *dir;
	struct dirent *ent;

	string_or_die(&path, "%s/%s", globals.path_prefix, BUNDLES_DIR);

	dir = opendir(path);
	if (dir) {
		while ((ent = readdir(dir))) {
			if (ent->d_name[0] == '.') {
				/* ignore dot files, in particular .MoM */
				continue;
			}
			if (ent->d_type == DT_REG) {
				if (component_subscribed(*subs, ent->d_name)) {
					/*  This is considered odd since means two files same name on same folder */
					continue;
				}

				if (strcmp(ent->d_name, "os-core") == 0) {
					have_os_core = true;
				}

				create_and_append_subscription(subs, ent->d_name);
			}
		}

		closedir(dir);
	}

	free_string(&path);

	/* Always add os-core */
	if (!have_os_core) {
		create_and_append_subscription(subs, "os-core");
	}

	*subs = list_sort(*subs, subscription_sort_component);
}

static bool set_subscription_obligation(struct list *subs, char *component, bool is_optional)
{
	struct list *list;
	struct sub *sub;

	list = list_head(subs);
	while (list) {
		sub = list->data;
		list = list->next;

		if (strcmp(sub->component, component) == 0) {
			sub->is_optional = is_optional;
			return true;
		}
	}

	return false;
}

bool component_subscribed(struct list *subs, char *component)
{
	struct list *list;
	struct sub *sub;

	list = list_head(subs);
	while (list) {
		sub = list->data;
		list = list->next;

		if (strcmp(sub->component, component) == 0) {
			return true;
		}
	}

	return false;
}

/* For the given subscription list (subs), set each subscription version as
 * listed in the MoM (latest). For 'update', the subscription versions from the
 * old MoM (current) are also set. For other subcommands, pass NULL for
 * current, since only a single MoM is referenced.
 */
void set_subscription_versions(struct manifest *latest, struct manifest *current, struct list **subs)
{
	struct list *list;
	struct file *file;
	struct sub *sub;

	list = list_head(*subs);
	while (list) {
		sub = list->data;
		list = list->next;

		file = mom_search_bundle(latest, sub->component);
		if (file) {
			sub->version = file->last_change;
		}

		if (current) {
			file = mom_search_bundle(current, sub->component);
			if (file) {
				sub->oldversion = file->last_change;
			}
		}
	}
}

static void create_new_subscription(struct list **subs, const char *component, bool is_optional)
{
	struct sub *sub;

	sub = calloc(1, sizeof(struct sub));
	ON_NULL_ABORT(sub);

	sub->component = strdup_or_die(component);

	sub->version = 0;
	sub->oldversion = 0;
	sub->is_optional = is_optional;
	*subs = list_prepend_data(*subs, sub);
}

void create_and_append_subscription(struct list **subs, const char *component)
{
	create_new_subscription(subs, component, false);
}

/* bitmapped return
   1 error happened
   2 new subscriptions
   4 bad name given
*/
static int recurse_subscriptions(struct list *bundles, struct list **subs, struct manifest *mom, bool find_all, int recursion, subs_fn_t subs_fn)
{
	char *bundle;
	int manifest_err;
	int ret = 0;
	struct file *file;
	struct list *iter;
	struct manifest *manifest;
	static bool is_optional = false;
	static bool force_optional = false;

	iter = list_head(bundles);
	while (iter) {
		bundle = iter->data;
		iter = iter->next;

		file = mom_search_bundle(mom, bundle);
		if (!file) {
			warn("Bundle \"%s\" is invalid, skipping it...\n", bundle);
			ret |= add_sub_BADNAME; /* Use this to get non-zero exit code */
			continue;
		}

		if (!find_all && is_installed_bundle(bundle)) {
			continue;
		}

		manifest = load_manifest(file->last_change, file, mom, true, &manifest_err);
		if (!manifest) {
			error("Unable to download manifest %s version %d, exiting now\n", bundle, file->last_change);
			ret |= add_sub_ERR;
			goto out;
		}

		if (!subs_fn(subs, bundle, recursion, force_optional | is_optional)) {
			/* nothing else to do with the current bundle */
			manifest_free(manifest);
			continue;
		}

		ret |= add_sub_NEW; /* We have gone through at least one bundle */

		if (!globals.skip_optional_bundles && manifest->optional) {
			/* merge in recursive call results, all bundles added via an optional
			 * include should be subscribed as optional */
			if (recursion == 0) {
				force_optional = true;
			}
			is_optional = true;
			ret |= recurse_subscriptions(manifest->optional, subs, mom, find_all, recursion + 1, subs_fn);
		}

		if (manifest->includes) {
			/* merge in recursive call results */
			if (recursion == 0) {
				force_optional = false;
			}
			is_optional = false;
			ret |= recurse_subscriptions(manifest->includes, subs, mom, find_all, recursion + 1, subs_fn);
		}

		manifest_free(manifest);
	}
out:
	return ret;
}

static bool create_subscriptions(struct list **subs, const char *component, int recursion, bool is_optional)
{
	/*
	 * If we're recursing a tree of includes, we need to cut out early
	 * if the bundle we're looking at is already subscribed...
	 * Because if it is, we'll visit it soon anyway at the top level.
	 *
	 * We can't do this for the toplevel of the recursion because
	 * that is how we initiallly fill in the include tree.
	 */
	if (set_subscription_obligation(*subs, (char *)component, is_optional)) {
		if (recursion > 0) {
			return false;
		}
		return true;
	}
	create_new_subscription(subs, component, is_optional);

	return true;
}

static bool print_subscriptions_tree(UNUSED_PARAM struct list **subs, const char *component, int recursion, UNUSED_PARAM bool is_optional)
{
	int indent = 0;

	if (recursion == 0) {
		info("%*s* %s\n", 2, "", component);
	} else {
		indent = recursion * 4;
		info("%*s|-- %s\n", indent, "", component);
	}

	return true;
}

int add_subscriptions(struct list *bundles, struct list **subs, struct manifest *mom, bool find_all, int recursion)
{
	return recurse_subscriptions(bundles, subs, mom, find_all, recursion, create_subscriptions);
}

int subscription_get_tree(struct list *bundles, struct list **subs, struct manifest *mom, bool find_all, int recursion)
{
	return recurse_subscriptions(bundles, subs, mom, find_all, recursion, print_subscriptions_tree);
}
