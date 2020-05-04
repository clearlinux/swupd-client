/*
 *   Software Updater - client side
 *
 *      Copyright (c) 2012-2020 Intel Corporation.
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

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "swupd_lib/alias.h"
#include "swupd.h"

#define MODE_RW_O (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

/* Finds out whether bundle_name is installed bundle on
*  current system.
*/
bool is_installed_bundle(const char *bundle_name)
{
	char *filename = NULL;
	bool ret = true;

	filename = sys_path_join("%s/%s/%s", globals.path_prefix, BUNDLES_DIR, bundle_name);
	ret = sys_file_exists(filename);
	FREE(filename);

	return ret;
}

bool is_tracked_bundle(const char *bundle_name)
{
	char *filename = NULL;
	bool ret;

	filename = sys_path_join("%s/bundles/%s", globals.state_dir, bundle_name);
	ret = sys_file_exists(filename);
	FREE(filename);

	return ret;
}

/* Return list of bundles that include bundle_name */
int required_by(struct list **reqd_by, const char *bundle_name, struct manifest *mom, int recursion, struct list *exclusions, char *msg, bool include_optional)
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

		if (str_cmp(bundle->component, bundle_name) == 0) {
			/* circular dependencies are not allowed in manifests,
			 * so we can skip checking for dependencies in the same bundle */
			continue;
		}

		int indent = 0;
		i = list_head(bundle->includes);
		while (i) {
			name = i->data;
			i = i->next;

			if (str_cmp(name, bundle_name) == 0) {

				/* this bundle has *bundle_name as a dependency */

				/* if the bundle being looked at is in the list of exclusions
				 * then don't consider it as a dependency, the user added it to
				 * the list of bundles to be removed too, but we DO want to
				 * consider its list of includes */
				if (!list_search(exclusions, bundle->component, str_cmp_wrapper)) {

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
				required_by(reqd_by, bundle->component, mom, recursion, exclusions, msg, include_optional);
			}
		}

		/* now look at the optional dependencies */
		if (include_optional) {
			i = list_head(bundle->optional);
			while (i) {
				name = i->data;
				i = i->next;

				if (str_cmp(name, bundle_name) == 0) {

					if (!list_search(exclusions, bundle->component, str_cmp_wrapper)) {

						char *bundle_str = NULL;
						string_or_die(&bundle_str, "%s (as optional)", bundle->component);
						*reqd_by = list_append_data(*reqd_by, bundle_str);

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
								info("%*s* %s (as optional)\n", indent + 2, "", bundle->component);
							} else {
								info("%*s|-- %s (as optional)\n", indent, "", bundle->component);
							}
						}
					}

					/* there is no need to recurse when lookin at the bundles that have
					 * the specified bundle as optional dependency since it may or may
					 * not include it, so it will only confuse */
				}
			}
		}
	}

	if (recursion == 1) {
		/* get rid of duplicated dependencies */
		*reqd_by = list_sort(*reqd_by, str_cmp_wrapper);
		*reqd_by = list_sorted_deduplicate(*reqd_by, str_cmp_wrapper, free);

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

	tracking_dir = sys_path_join("%s/%s", state_dir, "bundles");
	tracking_file = sys_path_join("%s/%s", tracking_dir, bundle_name);

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
	FREE(tracking_dir);
	FREE(tracking_file);
}

void track_bundle(const char *bundle_name)
{
	track_bundle_in_statedir(bundle_name, globals.state_dir);
}

static char *get_bundles_dir(void)
{
	return sys_path_join("%s/%s", globals.path_prefix, BUNDLES_DIR);
}

struct list *bundle_list_tracked(void)
{
	struct list *bundles = NULL;
	char *tracking_dir = get_tracking_dir();

	bundles = sys_ls(tracking_dir);
	FREE(tracking_dir);

	return bundles;
}

struct list *bundle_list_installed(void)
{
	struct list *bundles = NULL;
	char *bundles_dir = get_bundles_dir();

	bundles = sys_ls(bundles_dir);
	FREE(bundles_dir);

	return bundles;
}

enum swupd_code bundle_list_orphans(struct manifest *mom, struct list **bundles)
{
	struct list *tracked_bundles = NULL;
	struct list *subs = NULL;
	enum swupd_code ret_code = SWUPD_OK;
	int ret;

	// start with a list of the tracked bundles
	// (bundles specifically installed by the user)
	tracked_bundles = bundle_list_tracked();
	if (!tracked_bundles) {
		// this should never happen, swupd_init makes sure of it
		ret_code = SWUPD_COULDNT_LIST_DIR;
		goto out;
	}

	// create the list of required bundles by using tracked bundles as base
	// and adding the dependencies of each tracked bundles to the list
	ret = add_subscriptions(tracked_bundles, &subs, mom, true, 0);
	if (ret != add_sub_NEW) {
		// something went wrong or there were no includes
		if (ret & add_sub_ERR) {
			error("Cannot load included bundles\n\n");
			ret_code = SWUPD_COULDNT_LOAD_MANIFEST;
		} else {
			error("Unknown error\n\n");
			ret_code = SWUPD_UNEXPECTED_CONDITION;
		}
		goto out;
	}

	// create a list with all bundles installed in the system
	*bundles = bundle_list_installed();

	// sort the lists
	subs = list_sort(subs, cmp_subscription_component);
	*bundles = list_sort(*bundles, str_cmp_wrapper);

	// remove the bundles in the list of subscribed bundles from
	// the list of all bundles
	*bundles = list_sorted_filter_common_elements(*bundles, subs, cmp_string_sub_component, free);

out:
	list_free_list_and_data(tracked_bundles, free);
	free_subscriptions(&subs);
	return ret_code;
}
