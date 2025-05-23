/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2025 Intel Corporation.
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "signature.h"
#include "swupd.h"
#include "swupd_build_variant.h"

/* hysteresis thresholds */
/* TODO: update MAX_XFER after bug #562 is fixed.
 *
 * MAX XFER is the number of simultaneous downloads to be performed at the same
 * time. This value is set to 1 because of a bug that causes a dowload problem
 * when extrating a large file while other is being downloaded. Set this value
 * to a larger number, to be defined by tests, after bug is fixed.
 */
#define MAX_XFER 1

struct pack_data {
	char *url;
	char *filename;
	const char *module;
	int newversion;
	int oldversion;
};

static int finalize_pack_download(const char *module, int newversion, const char *filename)
{
	FILE *tarfile = NULL;
	char *delta_pack_dir = NULL;
	int err;

	delta_pack_dir = statedir_get_delta_pack_dir();
	debug("\nExtracting %s pack for version %i\n", module, newversion);
	err = archives_extract_to(filename, delta_pack_dir);

	unlink(filename);

	if (err == 0) {
		/* make a zero sized file to prevent redownload */
		tarfile = fopen(filename, "w");
		if (tarfile) {
			fclose(tarfile);
		}
	}

	FREE(delta_pack_dir);

	return err;
}

static int download_pack(int oldversion, int newversion, char *module)
{
	char *url = NULL;
	char *filename;
	int ret;

	filename = statedir_get_delta_pack(module, oldversion, newversion);

	string_or_die(&url, "%s/%i/pack-%s-from-%i.tar", globals.content_url, newversion, module, oldversion);

	ret = swupd_curl_get_file(url, filename);
	FREE(url);
	if (!ret) {
		ret = finalize_pack_download(module, newversion, filename);
	}
	FREE(filename);

	return ret;
}

static int64_t packs_query_total_download_size(struct list *subs, struct manifest *mom)
{
	int64_t size = 0;
	int64_t total_size = 0;
	struct sub *sub = NULL;
	struct list *list = NULL;
	struct file *bundle = NULL;
	char *url = NULL;
	int count = 0;

	for (list = list_head(subs); list; list = list->next) {
		sub = list->data;

		bundle = mom_search_bundle(mom, sub->component);
		if (!bundle) {
			debug("The manifest for bundle %s was not found in the MoM\n", sub->component);
			return -SWUPD_INVALID_BUNDLE;
		}

		string_or_die(&url, "%s/%i/pack-%s-from-%i.tar", globals.content_url, sub->version, sub->component, sub->oldversion);
		size = swupd_curl_query_content_size(url);
		if (size >= 0) {
			total_size += size;
		} else {
			debug("The pack header for bundle %s could not be downloaded\n", sub->component);
			FREE(url);
			return -SWUPD_COULDNT_DOWNLOAD_FILE;
		}

		count++;
		debug("Pack: %s (%.2lf MB)\n", url, size / 1000000);
		FREE(url);
	}

	debug("Number of packs to download: %d\n", count);
	debug("Total size of packs to be downloaded: %.2lf MB\n", total_size / 1000000);
	return total_size;
}

static int get_cached_packs(struct sub *sub)
{
	char *targetfile;
	char *targetfile_cache;
	struct stat stat;
	int ret = 0;

	/* Check the statedir and statedir-cache for the expected pack. When the
	 * pack exists in the statedir-cache, it is copied to the statedir. */
	targetfile = statedir_get_delta_pack(sub->component, sub->oldversion, sub->version);
	if (lstat(targetfile, &stat) != 0 || stat.st_size != 0) {
		ret = 1;

		if (globals.state_dir_cache != NULL) {
			targetfile_cache = statedir_dup_get_delta_pack(sub->component, sub->oldversion, sub->version);
			if (lstat(targetfile_cache, &stat) == 0 && stat.st_size == 0) {
				if (link_or_copy(targetfile_cache, targetfile) == 0) {
					ret = 0;
				}
			}
			FREE(targetfile_cache);
		}
	}
	FREE(targetfile);

	return ret;
}

/* pull in packs for base and any subscription */
int download_subscribed_packs(struct list *subs, struct manifest *mom, bool required)
{
	int ret = 0;
	struct list *iter;
	struct list *need_download = NULL;
	struct sub *sub = NULL;
	struct file *bundle = NULL;
	struct download_progress download_progress = { 0, 0 };
	int64_t download_size;
	int err;
	int list_length;
	int complete = 0;
	char *packs_size;

	progress_next_step("download_packs", PROGRESS_BAR);

	/* make a new list with only the bundles we actually need to download packs for */
	for (iter = list_head(subs); iter; iter = iter->next) {
		sub = iter->data;

		if (!is_installed_bundle(sub->component)) {
			/* if the bundle is not installed in the system but is in the subs list it
			 * means it was recently added as a dependency, so we need to fix the oldversion
			 * in the subscription so we download the correct pack */
			sub->oldversion = 0;
		}

		if (sub->oldversion == sub->version) {
			/* the bundle didn't change, we don't need to download */
			continue;
		}

		if (sub->oldversion > sub->version) {
			/* this condition could happen if for example the bundle does not exist
			 * anymore, so it was removed from the MoM in a latter version */
			continue;
		}

		/* make sure the file is not already in the client system */
		if (get_cached_packs(sub) != 0) {
			need_download = list_append_data(need_download, sub);
		}
	}

	if (!need_download) {
		/* no packs needs to be downloaded */
		info("No packs need to be downloaded\n");
		goto out;
	}

	/* get size of the packs to download */
	download_size = packs_query_total_download_size(need_download, mom);
	if (download_size < 0) {
		debug("Couldn't get the size of the packs to download, using number of packs instead\n");
		download_progress.total_download_size = 0;
	} else {
		download_progress.total_download_size = (uint64_t)download_size;
	}

	/* show the packs size only if > 1 MB */
	string_or_die(&packs_size, "(%.2lf MB) ", (double)download_progress.total_download_size / 1000000);
	info("Downloading packs %sfor:\n", ((double)download_progress.total_download_size / 1000000) > 1 ? packs_size : "");
	FREE(packs_size);
	for (iter = list_head(need_download); iter; iter = iter->next) {
		sub = iter->data;

		info(" - %s\n", sub->component);
	}

	list_length = list_len(need_download);
	for (iter = list_head(need_download); iter; iter = iter->next) {
		sub = iter->data;

		bundle = mom_search_bundle(mom, sub->component);
		if (!bundle) {
			debug("The manifest for bundle %s was not found in the MoM\n", sub->component);

			ret = -SWUPD_INVALID_BUNDLE;
			goto out;
		}

		err = download_pack(sub->oldversion, sub->version, sub->component);

		/* fall back for progress reporting when the download size
		 * could not be determined */
		if (download_progress.total_download_size == 0) {
			complete++;
			progress_report(complete, list_length);
		}
		if (err < 0 && required) {
			ret = err;
			goto out;
		}
	}
	list_free_list(need_download);
	info("Finishing packs extraction...\n");

	progress_next_step("extract_packs", PROGRESS_UNDEFINED);
	return ret;

out:
	list_free_list(need_download);
	progress_next_step("extract_packs", PROGRESS_UNDEFINED);
	return ret;
}

int download_zero_packs(struct list *bundles, struct manifest *mom)
{
	struct list *subs = NULL, *iter;
	int ret;

	for (iter = bundles; iter; iter = iter->next) {
		struct manifest *m = iter->data;
		struct sub *sub;

		sub = malloc_or_die(sizeof(struct sub));
		sub->component = m->component;
		sub->version = m->version;

		subs = list_prepend_data(subs, sub);
	}

	ret = download_subscribed_packs(subs, mom, false);

	list_free_list_and_data(subs, free);

	return ret;
}
