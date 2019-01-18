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

//TODO: Move to progress bar
static unsigned int complete = 0;
static unsigned int num_packs = 0;

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
	char *component;
	bool is_mix;
};

static int finalize_pack_download(const char *filename)
{
	FILE *tarfile = NULL;
	int err;

	debug("Extracting pack %s\n", filename);
	err = extract_to(filename, state_dir);

	unlink(filename);

	if (err == 0) {
		/* make a zero sized file to prevent redownload */
		tarfile = fopen(filename, "w");
		if (tarfile) {
			fclose(tarfile);
		}
		print_progress(++complete, num_packs);
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
	free_string(&pack_data->component);
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

	return finalize_pack_download(pack_data->filename) == 0;
}

static int download_pack(void *download_handle, struct pack_data *pack_data)
{
	int err = -1;

	if (pack_data->is_mix) {
		err = link(pack_data->url, pack_data->filename);
		if (!err) {
			debug("Pack linked: %s to %s\n", pack_data->url, pack_data->filename);

			err = finalize_pack_download(pack_data->filename);
		}
		download_free_data(pack_data);
	} else {
		err = swupd_curl_parallel_download_enqueue(download_handle, pack_data->url, pack_data->filename, NULL, pack_data);
	}

	return err;
}

static struct list *process_packs_to_download(struct list *subs, struct manifest *mom)
{
	struct list *packs = NULL, *i;

	for (i = list_head(subs); i; i = i->next) {
		char *filename;
		struct stat stat;
		struct pack_data *pack_data;
		struct sub *sub = i->data;
		int err;
		struct file *bundle;

		// Check if pack didn't change in this release
		if (sub->oldversion == sub->version) {
			continue;
		}


		// Check if pack was already downloaded
		string_or_die(&filename, "%s/pack-%s-from-%i-to-%i.tar",
			      state_dir, sub->component, sub->oldversion,
			      sub->version);

		err = lstat(filename, &stat);
		if (err == 0 && stat.st_size == 0) {
			free_string(&filename);
			continue;
		}

		// Pack should be downloaded
		pack_data = calloc(1, sizeof(struct pack_data));
		ON_NULL_ABORT(pack_data);

		pack_data->filename = filename;
		pack_data->component = strdup_or_die(sub->component);

		bundle = search_bundle_in_manifest(mom, sub->component);
		pack_data->is_mix = bundle ? !!bundle->is_mix : false;
		pack_data->url = str_or_die("%s/%i/pack-%s-from-%i.tar",
			pack_data->is_mix ? MIX_STATE_DIR : content_url,
			sub->version, sub->component, sub->oldversion);
		packs = list_prepend_data(packs, pack_data);
	}

	return packs;
}

/* pull in packs for base and any subscription */
int download_subscribed_packs(struct list *subs, struct manifest *mom)
{
	struct list *packs, *i;
	void *download_handle;
	int ret;

	packs = process_packs_to_download(subs, mom);
	if (!packs) {
		return 0; // no packs to download
	}

	print("Downloading packs for:\n");
	for (i = packs; i; i = i->next) {
		struct pack_data *pack_data = i->data;
		print("\t%s\n", pack_data->component);
	}
	print("\n");

	num_packs = list_len(packs);
	download_handle = swupd_curl_parallel_download_start(get_max_xfer(MAX_XFER));

	swupd_curl_parallel_download_set_callbacks(download_handle, download_successful, download_error, download_free_data);

	print_progress(0, num_packs);
	complete = 0;
	for (i = packs; i; i = i->next) {
		struct pack_data *pack_data = i->data;

		download_pack(download_handle, pack_data);
	}

	ret = swupd_curl_parallel_download_end(download_handle, NULL);
	print_progress(num_packs, num_packs); /* Force out 100% */
	print("\n")
	return ret;
}
