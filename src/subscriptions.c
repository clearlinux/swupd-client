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

void free_subscriptions(void)
{
	list_free_list_and_data(subs, free_subscription_data);
	subs = NULL;
}

struct list *free_bundle(struct list *item)
{
	return list_free_item(item, free_subscription_data);
}

struct list *free_list_file(struct list *item)
{
	return list_free_item(item, free_file_data);
}

void read_subscriptions_alt(void)
{
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
				if (component_subscribed(ent->d_name)) {
					/*  This is considered odd since means two files same name on same folder */
					continue;
				}

				create_and_append_subscription(ent->d_name);
			}
		}

		closedir(dir);
	}

	free(path);

	/* if nothing was picked up from bundles directory then add os-core by default */
	if (list_len(subs) == 0) {
		create_and_append_subscription("os-core");
	}
}

int component_subscribed(char *component)
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

int subscription_versions_from_MoM(struct manifest *MoM, int is_old)
{
	struct list *list;
	struct list *list2;
	struct file *file;
	struct sub *sub;
	bool bundle_found;
	int ret = 0;

	list = list_head(subs);
	while (list) {
		bundle_found = false;
		sub = list->data;
		list = list->next;

		list2 = MoM->manifests;
		while (list2 && !bundle_found) {
			file = list2->data;
			list2 = list2->next;

			if (strcmp(sub->component, file->filename) == 0) {
				if (is_old) {
					sub->oldversion = file->last_change;
				} else {
					sub->version = file->last_change;
				}
				bundle_found = true;
			}
		}

		if (!bundle_found) {
			sub->version = -1;
			printf("ERROR: Bundle not in MoM |\"component=%s\"\n", sub->component);
			ret = EBUNDLE_MISMATCH;
		}
	}

	return ret;
}

void create_and_append_subscription(const char *component)
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
	subs = list_prepend_data(subs, sub);
}
