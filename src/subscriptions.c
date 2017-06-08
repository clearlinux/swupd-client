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

	free(sub->component);
	free(sub);
}

void free_subscriptions(struct list **subs)
{
	list_free_list_and_data(*subs, free_subscription_data);
	*subs = NULL;
}

struct list *free_bundle(struct list *item)
{
	return list_free_item(item, free_subscription_data);
}

struct list *free_list_file(struct list *item)
{
	return list_free_item(item, free_file_data);
}

static int subscription_sort_component(const void *a, const void *b)
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

	string_or_die(&path, "%s/%s", path_prefix, BUNDLES_DIR);

	dir = opendir(path);
	if (dir) {
		while ((ent = readdir(dir))) {
			if ((strcmp(ent->d_name, ".") == 0) || (strcmp(ent->d_name, "..") == 0)) {
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

	free(path);

	/* Always add os-core */
	if (!have_os_core) {
		create_and_append_subscription(subs, "os-core");
	}

	*subs = list_sort(*subs, subscription_sort_component);
}

int component_subscribed(struct list *subs, char *component)
{
	struct list *list;
	struct sub *sub;

	list = list_head(subs);
	while (list) {
		sub = list->data;
		list = list->next;

		if (strcmp(sub->component, component) == 0) {
			return 1;
		}
	}

	return 0;
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

		file = search_bundle_in_manifest(latest, sub->component);
		if (file) {
			sub->version = file->last_change;
		}

		if (current) {
			file = search_bundle_in_manifest(current, sub->component);
			if (file) {
				sub->oldversion = file->last_change;
			}
		}
	}
}

void create_and_append_subscription(struct list **subs, const char *component)
{
	struct sub *sub;

	sub = calloc(1, sizeof(struct sub));
	if (!sub) {
		abort();
	}

	sub->component = strdup(component);
	if (sub->component == NULL) {
		abort();
	}

	sub->version = 0;
	sub->oldversion = 0;
	*subs = list_prepend_data(*subs, sub);
}
