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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "signature.h"
#include "swupd-build-variant.h"
#include "swupd.h"

static int download_pack(int oldversion, int newversion, char *module)
{
	FILE *tarfile = NULL;
	char *tar = NULL;
	char *url = NULL;
	int err = -1;
	char *filename;
	struct stat stat;

	printf("Downloading %s pack for version %i\n", module, newversion);

	string_or_die(&filename, "%s/pack-%s-from-%i-to-%i.tar", state_dir, module, oldversion, newversion);
	string_or_die(&url, "%s/%i/pack-%s-from-%i.tar", content_url, newversion, module, oldversion);

	err = swupd_curl_get_file(url, filename, NULL, NULL, true);
	if (err) {
		free(url);
		if ((lstat(filename, &stat) == 0) && (stat.st_size == 0)) {
			unlink(filename);
		}
		free(filename);
		return err;
	}

	free(url);

	printf("Extracting pack.\n");
	string_or_die(&tar, TAR_COMMAND " -C %s " TAR_PERM_ATTR_ARGS " -xf %s/pack-%s-from-%i-to-%i.tar 2> /dev/null",
		      state_dir, state_dir, module, oldversion, newversion);

	err = system(tar);
	if (WIFEXITED(err)) {
		err = WEXITSTATUS(err);
	}
	free(tar);
	unlink(filename);
	/* make a zero sized file to prevent redownload */
	tarfile = fopen(filename, "w");
	free(filename);
	if (tarfile) {
		fclose(tarfile);
	}

	return err;
}

/* strip out bundles from download list which have been prev installed,
	or which have not changed since oldversion */
struct list *get_min_bundle_list(struct list *full_list)
{
	struct list *min_list = list_clone(full_list);
	struct sub *bundle = NULL;
	char *file = NULL;
	struct list *list;
	struct list *next;
	int err = -1;
	struct stat stat;

	list = list_head(full_list);
	while (list) {
		bundle = list->data;
		next = list->next;

		/* bundle has not changed, no pack download needed */
		if (bundle->oldversion == bundle->version) {
			list_free_item(list, NULL);
			list = next;
			continue;
		}

		/* now check if the appropriate bundle update has been previously installed. Once installed,
			pack contents are truncated to 0 but the file left on fs as a hint */
		string_or_die(&file, "%s/pack-%s-from-%i-to-%i.tar", state_dir, bundle->component,
						bundle->oldversion, bundle->version);
		err = lstat(file, &stat);

		if (err == 0 && stat.st_size == 0) {
			list_free_item(list, NULL);
			list = next;
			free(file);
		}
		list = next;
	}

	return min_list;
}

/* sum size of required downloads */
bool space_check(struct list *bundles)
{
	struct sub *bundle = NULL;
	struct list *iter;
	struct statvfs stat;
	char *url = NULL;
	double req = 0, avail = 0, ret;

	/* sum required space for list of bundles */
	iter = list_head(bundles);
	while (iter) {
		bundle = iter->data;
		iter = iter->next;

		string_or_die(&url, "%s/%i/pack-%s-from-%i.tar", content_url, bundle->version,
						bundle->component, bundle->oldversion);
		ret = swupd_query_url_content_size(url);
		if (!ret) {
			req += ret;
		} else {
			printf("Unable to query size of: %s\n", url);
			return true; /* cannot estimate req'd space; must continue with download */
		}

		free(url);
	}

	/* check available disk space */
	if(statvfs(state_dir, &stat) != 0) {
		return true;
	}

	avail = stat.f_bsize * stat.f_bavail;
	if (req >= avail) {
		printf("Space check failed. %.2lf MB needed, only %.2lf available\n",
					req/1000000, avail/1000000);
		return false;
	}

	return true;
}

/* pull in packs for base and any subscription */
int download_subscribed_packs(bool required)
{
	struct list *iter;
	struct list *req_bundles; /* bundles requiring download */;
	struct sub *bundle = NULL;
	int err, ret = 0;

	if (!check_network()) {
		return -ENOSWUPDSERVER;
	}

	req_bundles = get_min_bundle_list(subs);
	if (!space_check(req_bundles)) {
		return -EINSUFFICIENT_SPACE;
	}

	iter = list_head(req_bundles);
	while (iter) {
		bundle = iter->data;
		iter = iter->next;

		err = download_pack(bundle->oldversion, bundle->version, bundle->component);
		if (err < 0) {
			if (required) {
				ret = err;
				break;
			} else {
				continue;
			}
		}
	}

	if (req_bundles) {
		list_free_list_and_data(req_bundles, free_subscription_data);
		req_bundles = NULL;
	}

	return 0;
}
