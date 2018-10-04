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
#include "swupd-build-variant.h"
#include "swupd.h"

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

	fprintf(stderr, "\nExtracting %s pack for version %i\n", module, newversion);
	err = extract_to(filename, state_dir);

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

static bool download_error(int response, void *data)
{
	struct pack_data *pack_data = data;

	if (!data) {
		return false;
	}

	if (response == 404) {
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

static int download_pack(void *download_handle, int oldversion, int newversion, char *module, int is_mix)
{
	char *url = NULL;
	int err = -1;
	char *filename;
	struct stat stat;

	string_or_die(&filename, "%s/pack-%s-from-%i-to-%i.tar", state_dir, module, oldversion, newversion);

	err = lstat(filename, &stat);
	if (err == 0 && stat.st_size == 0) {
		free_string(&filename);
		return 0;
	}

	if (is_mix) {
		string_or_die(&url, "%s/%i/pack-%s-from-%i.tar", MIX_STATE_DIR, newversion, module, oldversion);
		err = link(url, filename);
		if (err) {
			free_string(&filename);
			free_string(&url);
			return err;
		}
		printf("Linked %s to %s\n", url, filename);

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

/* pull in packs for base and any subscription */
int download_subscribed_packs(struct list *subs, struct manifest *mom, bool required)
{
	struct list *iter;
	struct sub *sub = NULL;
	int err;
	int is_mix = 0;
	unsigned int list_length = list_len(subs);
	unsigned int complete = 0;
	void *download_handle;

	fprintf(stderr, "Downloading packs...\n");
	download_handle = swupd_curl_parallel_download_start(get_max_xfer(MAX_XFER));

	swupd_curl_parallel_download_set_callbacks(download_handle, download_successful, download_error, download_free_data);
	iter = list_head(subs);
	while (iter) {
		sub = iter->data;
		iter = iter->next;
		complete++;

		if (sub->oldversion == sub->version) { // pack didn't change in this release
			continue;
		}

		struct file *bundle = search_bundle_in_manifest(mom, sub->component);
		if (bundle) {
			is_mix = bundle->is_mix;
		}
		err = download_pack(download_handle, sub->oldversion, sub->version, sub->component, is_mix);
		print_progress(complete, list_length);
		if (err < 0) {
			if (required) { /* Probably need printf("\n") here */
				return err;
			} else {
				continue;
			}
		}
	}

	print_progress(list_length, list_length); /* Force out 100% */
	printf("\n");
	return swupd_curl_parallel_download_end(download_handle, NULL);
}
