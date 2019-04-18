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
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
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
};

static int finalize_pack_download(const char *module, int newversion, const char *filename)
{
	FILE *tarfile = NULL;
	int err;

	debug("\nExtracting %s pack for version %i\n", module, newversion);
	err = archives_extract_to(filename, state_dir);

	unlink(filename);

	if (err == 0) {
		/* make a zero sized file to prevent redownload */
		tarfile = fopen(filename, "w");
		if (tarfile) {
			fclose(tarfile);
		}
	}

	return err;
}

static void download_free_data(void *data)
{
	struct pack_data *pack_data = data;

	if (!data) {
		return;
	}

	free_string(&pack_data->url);
	free_string(&pack_data->filename);
	free(pack_data);
}

static bool download_error(enum download_status status, void *data)
{
	struct pack_data *pack_data = data;

	if (!data) {
		return false;
	}

	if (status == DOWNLOAD_STATUS_NOT_FOUND) {
		telemetry(TELEMETRY_WARN, "packmissing", "url=%s\n", pack_data->url);
		return true;
	}

	return false;
}

static bool download_successful(void *data)
{
	struct pack_data *pack_data = data;

	if (!pack_data) {
		return false;
	}

	return finalize_pack_download(pack_data->module, pack_data->newversion, pack_data->filename) == 0;
}

static int download_pack(struct swupd_curl_parallel_handle *download_handle, int oldversion, int newversion, char *module, int is_mix)
{
	char *url = NULL;
	int err = -1;
	char *filename;

	string_or_die(&filename, "%s/pack-%s-from-%i-to-%i.tar", state_dir, module, oldversion, newversion);

	if (is_mix) {
		string_or_die(&url, "%s/%i/pack-%s-from-%i.tar", MIX_STATE_DIR, newversion, module, oldversion);
		err = link(url, filename);
		if (err) {
			free_string(&filename);
			free_string(&url);
			return err;
		}
		info("Linked %s to %s\n", url, filename);

		err = finalize_pack_download(module, newversion, filename);
		free_string(&url);
		free_string(&filename);
	} else {
		struct pack_data *pack_data;

		string_or_die(&url, "%s/%i/pack-%s-from-%i.tar", content_url, newversion, module, oldversion);

		pack_data = calloc(1, sizeof(struct pack_data));
		ON_NULL_ABORT(pack_data);

		pack_data->url = url;
		pack_data->filename = filename;
		pack_data->module = module;
		pack_data->newversion = newversion;

		err = swupd_curl_parallel_download_enqueue(download_handle, url, filename, NULL, pack_data);
	}

	return err;
}

static double packs_query_total_download_size(struct list *subs, struct manifest *mom)
{
	double size = 0;
	double total_size = 0;
	struct sub *sub = NULL;
	struct list *list = NULL;
	struct file *bundle = NULL;
	char *url = NULL;
	int count = 0;

	for (list = list_head(subs); list; list = list->next) {
		sub = list->data;

		/* if it is a pack from a mix, we won't download it */
		bundle = search_bundle_in_manifest(mom, sub->component);
		if (!bundle) {
			debug("The manifest for bundle %s was not found in the MoM", sub->component);
			return -SWUPD_INVALID_BUNDLE;
		}
		if (bundle->is_mix) {
			continue;
		}

		string_or_die(&url, "%s/%i/pack-%s-from-%i.tar", content_url, sub->version, sub->component, sub->oldversion);
		size = swupd_curl_query_content_size(url);
		if (size != -1) {
			total_size += size;
		} else {
			debug("The pack header for bundle %s could not be downloaded\n", sub->component);
			free_string(&url);
			return -SWUPD_COULDNT_DOWNLOAD_FILE;
		}

		count++;
		debug("Pack: %s (%.2lf Mb)\n", url, size / 1000000);
		free_string(&url);
	}

	debug("Number of packs to download: %d\n", count);
	debug("Total size of packs to be downloaded: %.2lf Mb\n", total_size / 1000000);
	return total_size;
}

/* pull in packs for base and any subscription */
int download_subscribed_packs(struct list *subs, struct manifest *mom, bool required)
{
	struct list *iter;
	struct list *need_download = NULL;
	struct sub *sub = NULL;
	struct stat stat;
	struct file *bundle = NULL;
	struct download_progress download_progress = { 0, 0, 0 };
	int err;
	unsigned int list_length;
	unsigned int complete = 0;
	struct swupd_curl_parallel_handle *download_handle;
	char *packs_size;

	/* make a new list with only the bundles we actually need to download packs for */
	for (iter = list_head(subs); iter; iter = iter->next) {
		char *targetfile;
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
		string_or_die(&targetfile, "%s/pack-%s-from-%i-to-%i.tar", state_dir, sub->component, sub->oldversion, sub->version);
		if (lstat(targetfile, &stat) != 0 || stat.st_size != 0) {
			need_download = list_append_data(need_download, sub);
		}

		free_string(&targetfile);
	}

	if (!need_download) {
		/* no packs needs to be downloaded */
		info("No packs need to be downloaded\n");
		progress_complete_step();
		return 0;
	}

	/* we need to download some files, so set up curl */
	download_handle = swupd_curl_parallel_download_start(get_max_xfer(MAX_XFER));
	swupd_curl_parallel_download_set_callbacks(download_handle, download_successful, download_error, download_free_data);

	/* get size of the packs to download */
	download_progress.total_download_size = packs_query_total_download_size(need_download, mom);
	if (download_progress.total_download_size > 0) {
		swupd_curl_parallel_download_set_progress_callbacks(download_handle, swupd_progress_callback, &download_progress);
	} else {
		debug("Couldn't get the size of the packs to download, using number of packs instead\n");
		download_progress.total_download_size = 0;
	}

	/* show the packs size only if > 1 Mb */
	string_or_die(&packs_size, "(%.2lf Mb) ", download_progress.total_download_size / 1000000);
	info("Downloading packs %sfor:\n", (download_progress.total_download_size / 1000000) > 1 ? packs_size : "");
	free_string(&packs_size);
	for (iter = list_head(need_download); iter; iter = iter->next) {
		sub = iter->data;

		info(" - %s\n", sub->component);
	}

	list_length = list_len(need_download);
	for (iter = list_head(need_download); iter; iter = iter->next) {
		sub = iter->data;

		bundle = search_bundle_in_manifest(mom, sub->component);
		if (!bundle) {
			debug("The manifest for bundle %s was not found in the MoM", sub->component);
			return -SWUPD_INVALID_BUNDLE;
		}

		err = download_pack(download_handle, sub->oldversion, sub->version, sub->component, bundle->is_mix);

		/* fall back for progress reporting when the download size
		* could not be determined */
		if (download_progress.total_download_size == 0) {
			complete++;
			progress_report(complete, list_length);
		}
		if (err < 0) {
			if (required) { /* Probably need printf("\n") here */
				return err;
			} else {
				continue;
			}
		}
	}
	info("\n");
	list_free_list(need_download);

	return swupd_curl_parallel_download_end(download_handle, NULL);
}
