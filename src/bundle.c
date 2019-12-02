/*
*   Software Updater - client side
*
*      Copyright (c) 2012-2016 Intel Corporation.
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
*         Jaime A. Garcia <jaime.garcia.naranjo@linux.intel.com>
*         Tim Pepper <timothy.c.pepper@linux.intel.com>
*
*/

#define _GNU_SOURCE
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "alias.h"
#include "config.h"
#include "swupd.h"

#define MODE_RW_O (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

/* Finds out whether bundle_name is installed bundle on
*  current system.
*/
bool is_installed_bundle(const char *bundle_name)
{
	char *filename = NULL;
	bool ret = true;

	string_or_die(&filename, "%s/%s/%s", globals.path_prefix, BUNDLES_DIR, bundle_name);
	ret = sys_file_exists(filename);
	free_string(&filename);

	return ret;
}

bool is_tracked_bundle(const char *bundle_name)
{
	char *filename = NULL;
	bool ret;

	string_or_die(&filename, "%s/bundles/%s", globals.state_dir, bundle_name);
	ret = sys_file_exists(filename);
	free_string(&filename);

	return ret;
}
/* Return list of bundles that include bundle_name */
int required_by(struct list **reqd_by, const char *bundle_name, struct manifest *mom, int recursion, struct list *exclusions, char *msg)
{
	struct list *b, *i;
	char *name;
	int count = 0;
	static bool print_msg;
	bool verbose = (log_get_level() == LOG_INFO_VERBOSE);

	// track recursion level for indentation
	if (recursion == 0) {
		print_msg = true;
	}
	recursion++;

	/* look at the manifest of all listed bundles to see if
	 * they list *bundle_name as a dependency */
	b = list_head(mom->submanifests);
	while (b) {
		struct manifest *bundle = b->data;
		b = b->next;

		if (strcmp(bundle->component, bundle_name) == 0) {
			/* circular dependencies are not allowed in manifests,
			 * so we can skip checking for dependencies in the same bundle */
			continue;
		}

		int indent = 0;
		i = list_head(bundle->includes);
		while (i) {
			name = i->data;
			i = i->next;

			if (strcmp(name, bundle_name) == 0) {

				/* this bundle has *bundle_name as a dependency */

				/* if the bundle being looked at is in the list of exclusions
				 * then don't consider it as a dependency, the user added it to
				 * the list of bundles to be removed too, but we DO want to
				 * consider its list of includes */
				if (!list_search(exclusions, bundle->component, strcmp_wrapper)) {

					/* add bundle to list of dependencies */
					char *bundle_str = NULL;
					string_or_die(&bundle_str, "%s", bundle->component);
					*reqd_by = list_append_data(*reqd_by, bundle_str);

					/* if the --verbose options was used, print the dependency as a
					 * tree element, in this view it is ok to have duplicated elements */
					if (verbose) {
						if (print_msg) {
							/* these messages should be printed only once */
							print_msg = false;
							info("%s", msg);
							info("\nformat:\n");
							info(" # * is-required-by\n");
							info(" #   |-- is-required-by\n");
							info(" # * is-also-required-by\n # ...\n");
							info("\n");
						}
						indent = (recursion - 1) * 4;
						if (recursion == 1) {
							info("%*s* %s\n", indent + 2, "", bundle->component);
						} else {
							info("%*s|-- %s\n", indent, "", bundle->component);
						}
					}
				}

				/* let's see what bundles list this new bundle as a dependency */
				required_by(reqd_by, bundle->component, mom, recursion, exclusions, msg);
			}
		}
	}

	if (recursion == 1) {
		/* get rid of duplicated dependencies */
		*reqd_by = list_sort(*reqd_by, strcmp_wrapper);
		*reqd_by = list_sorted_deduplicate(*reqd_by, strcmp_wrapper, free);

		/* if not using --verbose, we need to print the simplified
		 * list of bundles that depend on *bundle_name */
		i = list_head(*reqd_by);
		while (i) {
			name = i->data;
			count++;
			i = i->next;

			if (!verbose) {
				if (print_msg) {
					/* this message should be printed only once */
					print_msg = false;
					info("%s", msg);
				}
				info(" - ");
				print("%s\n", name);
			}
		}
	}

	return count;
}

void track_bundle_in_statedir(const char *bundle_name, const char *state_dir)
{
	int ret = 0;
	int fd;
	char *tracking_dir;
	char *tracking_file;

	tracking_dir = sys_path_join(state_dir, "bundles");
	tracking_file = sys_path_join(tracking_dir, bundle_name);

	/* touch a tracking file */
	fd = open(tracking_file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		ret = -1;
		goto out;
	}
	close(fd);

out:
	if (ret) {
		debug("Issue creating tracking file in %s for %s\n", tracking_dir, bundle_name);
	}
	free_string(&tracking_dir);
	free_string(&tracking_file);
}

void track_bundle(const char *bundle_name)
{
	track_bundle_in_statedir(bundle_name, globals.state_dir);
}
