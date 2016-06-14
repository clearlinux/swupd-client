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

static int download_pack(int oldversion, int newversion, char *module)
{
	FILE *tarfile = NULL;
	char *tar = NULL;
	char *url = NULL;
	int err = -1;
	char *filename;
	struct stat stat;

	string_or_die(&filename, "%s/pack-%s-from-%i-to-%i.tar", state_dir, module, oldversion, newversion);

	err = lstat(filename, &stat);
	if (err == 0 && stat.st_size == 0) {
		free(filename);
		return 0;
	}

	printf("Downloading %s pack for version %i\n", module, newversion);

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

/* pull in packs for base and any subscription */
int download_subscribed_packs(bool required)
{
	struct list *iter;
	struct sub *sub = NULL;
	int err;

	if (!check_network()) {
		return -ENOSWUPDSERVER;
	}

	iter = list_head(subs);
	while (iter) {
		sub = iter->data;
		iter = iter->next;

		if (sub->oldversion == sub->version) { // pack didn't change in this release
			continue;
		}

		err = download_pack(sub->oldversion, sub->version, sub->component);
		if (err < 0) {
			if (required) {
				return err;
			} else {
				continue;
			}
		}
	}

	return 0;
}
