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
 *         Otavio Pontes <otavio.pontes@intel.com>
 *
 */

#define _GNU_SOURCE
#include <unistd.h>

#include "swupd.h"

static struct list *download_loop(struct list *files, bool free_list)
{
	int ret;
	struct list *iter;
	unsigned int complete = 0;
	unsigned int list_length = list_len(files);

	iter = list_head(files);
	while (iter) {
		struct file *file;

		file = iter->data;
		iter = iter->next;
		complete++;

		if (file->is_deleted || file->do_not_update) {
			continue;
		}

		/* Mix content is local, so don't queue files up for curl downloads */
		if (file->is_mix) {
			char *url;

			if (file->staging) {
				free_string(&file->staging);
			}

			string_or_die(&url, "%s/%i/files/%s.tar", MIX_STATE_DIR, file->last_change, file->hash);
			string_or_die(&file->staging, "%s/download/.%s.tar", state_dir, file->hash);

			ret = link(url, file->staging);
			/* Try doing a regular rename if hardlink fails */
			if (ret) {
				if (rename(url, file->staging) != 0) {
					fprintf(stderr, "Failed to copy local mix file: %s\n", file->staging);
					continue;
				}
			}
			untar_full_download(file);
			free_string(&url);
			continue;
		}
		full_download(file);
		print_progress(complete, list_length);
	}
	print_progress(list_length, list_length); /* Force out 100% */
	printf("\n");

	if (free_list) {
		list_free_list(files);
	}

	return end_full_download();
}

/*
 * Download all fullfiles from the files list.
 * Return 0 on success or a negative number if any file was not downloaded successfully
 */
int download_fullfiles(struct list *files, int num_retries, int timeout)
{
	int ret;
	int retries = 0;

	while (files) {
		ret = start_full_download();
		if (ret != 0) {
			/* If we hit this point, the network is accessible but we were
			 * unable to download the needed files. This is a terminal error
			 * and we need good logging */
			return -EFULLDOWNLOAD;
		}

		files = download_loop(files, retries > 0);

		/* Set retries only if failed downloads exist, and only retry a fixed
		   amount of &times */
		if (list_head(files)) {
			if (retries >= num_retries) {
				list_free_list(files);
				return -EFULLDOWNLOAD;
			}

			increment_retries(&retries, &timeout);
			fprintf(stderr, "Starting download retry #%d\n", retries);
		}
	}

	return 0;
}
