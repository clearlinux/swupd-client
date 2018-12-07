/*
*   Software Updater - client side
*
*      Copyright (c) 2018 Intel Corporation.
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
*         William Douglas <william.douglas@intel.com>
*
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "alias.h"
#include "config.h"
#include "swupd.h"

/* Deep free on alias_lookup structs
 */
void free_alias_lookup(void *lookup)
{
	struct alias_lookup *l = (struct alias_lookup *)lookup;
	free(l->alias);
	list_free_list_and_data(l->bundles, free);
	free(l);
}

/* Get bundles for a given alias but if the alias is
 * not actually an alias, return a list with just the
 * bundle name
 */
struct list *get_alias_bundles(struct list *alias_definitions, char *alias)
{
	struct list *bundles = NULL;
	struct list *iter = alias_definitions;

	while (iter) {
		struct alias_lookup *lookup = (struct alias_lookup *)iter->data;
		iter = iter->next;
		if (strcmp(lookup->alias, alias) == 0) {
			bundles = list_deep_clone_strs(lookup->bundles);
			break;
		}
	}
	if (!bundles) {
		bundles = list_prepend_data(bundles, strdup_or_die(alias));
	}

	return bundles;
}

/* Parse an alias definition file for a list of bundles
 * it specifies
 *
 * Example file:
 * alias1	b1	b2
 * alias2	b3	b4
 *
 * If two files provide the same alias, the first one that
 * is parsed is the one that will be used.
 */
static struct list *parse_alias_file(char *fullpath)
{
	FILE *afile = NULL;
	struct list *alias_content = NULL;
	char *line = NULL;
	size_t len = 0;
	ssize_t read = -1;

	afile = fopen(fullpath, "r");
	if (!afile) {
		return NULL;
	}

	while ((read = getline(&line, &len, afile)) != -1) {
		struct alias_lookup *l = NULL;
		char *c = NULL;

		/* ignore lines starting with '#' */
		if (line[0] == '#') {
			continue;
		}

		/* get the alias but ignore lines without bundles */
		c = strchr(line, '\t');
		if (!c) {
			continue;
		}

		/* strip the newline if it exists */
		if (line[read - 1] == '\n') {
			line[read - 1] = 0;
		}

		*c = 0;
		c++;

		l = calloc(1, sizeof(struct alias_lookup));
		ON_NULL_ABORT(l);

		/* get the bundles associated with the alias
		 * note a file with multiple copies of the same
		 * alias will be first alias defintion wins */
		while (c && *c) {
			char *next = strchr(c, '\t');
			char *bundle = NULL;
			/* two tabs in a row, skip */
			if (c == next) {
				c++;
				continue;
			}
			if (next) {
				*next = 0;
				next++;
			}
			bundle = strdup_or_die(c);
			l->bundles = list_prepend_data(l->bundles, bundle);
			c = next;
		}
		if (!l->bundles) {
			free(l);
			continue;
		}
		l->alias = strdup_or_die(line);
		alias_content = list_prepend_data(alias_content, l);
	}

	if (!feof(afile)) {
		list_free_list_and_data(alias_content, free_alias_lookup);
		alias_content = NULL;
	}
	(void)fclose(afile);

	if (line) {
		free(line);
	}
	return alias_content;
}

/* Get alias definitions using both system and user
 * alias definition files
 *
 * The alias definition list will prioritize files by
 * in USER_ALIAS_PATH over SYSTEM_ALIAS_PATH, if files
 * share the same name in both, the file in the
 * USER_ALIAS_PATH will mask the file in the
 * SYSTEM_ALIAS_PATH. The list will then prioritize files
 * by lexicographicly sorting filenames.
 */
struct list *get_alias_definitions(void)
{
	char *path = NULL;
	struct list *combined = NULL;
	struct list *definitions = NULL;
	struct list *iters = NULL;
	struct list *iteru = NULL;
	struct list *system_alias_files = NULL;
	struct list *user_alias_files = NULL;

	/* get sorted system and user filenames */
	string_or_die(&path, "%s/%s", path_prefix, SYSTEM_ALIAS_PATH);
	system_alias_files = get_dir_files_sorted(path);
	free(path);

	string_or_die(&path, "%s/%s", path_prefix, USER_ALIAS_PATH);
	user_alias_files = get_dir_files_sorted(path);
	free(path);

	/* get a combined list with user files overriding system files */
	iters = system_alias_files;
	iteru = user_alias_files;
	while (iters && iteru) {
		int pivot = strcmp(basename(iteru->data), basename(iters->data));
		if (pivot == 0) {
			if (iters == system_alias_files) {
				system_alias_files = iters->next;
			}
			iters = list_free_item(iters, free);
			iteru = iteru->next;
		} else if (pivot < 0) {
			iteru = iteru->next;
		} else {
			iters = iters->next;
		}
	}

	combined = list_concat(user_alias_files, system_alias_files);

	/* read alias definitions */
	iteru = combined;

	while (iteru) {
		struct list *defns = NULL;
		defns = parse_alias_file(iteru->data);
		iteru = iteru->next;
		definitions = list_concat(definitions, defns);
	}
	list_free_list_and_data(combined, free);

	return definitions;
}
