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

static int download_pack(int oldversion, int newversion, char *module, int is_mix, bool resume_ok)
{
	FILE *tarfile = NULL;
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
		free_string(&url);
	} else {
		string_or_die(&url, "%s/%i/pack-%s-from-%i.tar", content_url, newversion, module, oldversion);

		err = swupd_curl_get_file_full(url, filename, NULL, resume_ok);
		if (err) {
			if (err == -ENET404) {
				telemetry(TELEMETRY_WARN, "packmissing",
					  "url=%s\n", url);
			}
			free_string(&url);
			if ((lstat(filename, &stat) == 0) && (stat.st_size == 0)) {
				unlink(filename);
			}
			free_string(&filename);
			return err;
		}
		free_string(&url);
	}

	fprintf(stderr, "\nExtracting %s pack for version %i\n", module, newversion);
	err = extract_to(filename, state_dir);

	unlink(filename);
	/* make a zero sized file to prevent redownload */
	tarfile = fopen(filename, "w");
	free_string(&filename);
	if (tarfile) {
		fclose(tarfile);
	}

	// Only negative return values should indicate errors
	if (err > 0) {
		return -err;
	} else {
		return err;
	}
}

/* pull in packs for base and any subscription */
int download_subscribed_packs(struct list *subs, struct manifest *mom, bool required, bool resume_ok)
{
	struct list *iter;
	struct sub *sub = NULL;
	int err;
	int is_mix = 0;
	unsigned int list_length = list_len(subs);
	unsigned int complete = 0;

	int ret = swupd_curl_check_network();
	if (ret) {
		return -ret;
	}

	fprintf(stderr, "Downloading packs...\n");
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
		err = download_pack(sub->oldversion, sub->version, sub->component, is_mix, resume_ok);
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
	return 0;
}
