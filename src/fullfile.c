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
#include <sys/stat.h>
#include <unistd.h>

#include "swupd.h"

/* hysteresis thresholds */
//FIXME #562
#define MAX_XFER 15

static void download_mix_file(struct file *file)
{
	char *url, *filename;

	string_or_die(&url, "%s/%i/files/%s.tar", MIX_STATE_DIR, file->last_change, file->hash);
	string_or_die(&filename, "%s/download/.%s.tar", state_dir, file->hash);

	/* Mix content is local, so don't queue files up for curl downloads */
	if (link_or_rename(url, filename) == 0) {
		untar_full_download(file);
	} else {
		info("Failed to copy local mix file: %s\n", file->staging);
	}

	free_string(&url);
	free_string(&filename);
}

static void download_file(void *download_handle, struct file *file)
{
	char *url, *filename;
	char *targetfile;
	struct stat stat;

	string_or_die(&targetfile, "%s/staged/%s", state_dir, file->hash);

	// Just download if the target file doesn't exist
	if (lstat(targetfile, &stat) != 0 || !verify_file(file, targetfile)) {
		string_or_die(&filename, "%s/download/.%s.tar", state_dir, file->hash);
		string_or_die(&url, "%s/%i/files/%s.tar", content_url, file->last_change, file->hash);
		swupd_curl_parallel_download_enqueue(download_handle, url, filename, file->hash, file);
		free_string(&url);
		free_string(&filename);
	}
	free_string(&targetfile);
}

static int download_loop(void *download_handle, struct list *files, int *num_downloads)
{
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

		if (file->is_mix) {
			download_mix_file(file);
		} else {
			download_file(download_handle, file);
		}
		print_progress(complete, list_length);
	}
	print_progress(list_length, list_length); /* Force out 100% */
	printf("\n");

	info("Finishing download of update content...\n");
	return swupd_curl_parallel_download_end(download_handle, num_downloads);
}

static bool download_error(enum download_status status, UNUSED_PARAM void *data)
{
	return status == DOWNLOAD_STATUS_NOT_FOUND;
}

static bool download_successful(void *data)
{
	if (!data) {
		return false;
	}

	if (untar_full_download(data) != 0) {
		info("Error for %s tarfile extraction, (check free space for %s?)\n",
		     ((struct file *)data)->hash, state_dir);
	}
	return true;
}

/*
 * Download fullfiles from the list of files.
 *
 * Return 0 on success or a negative number or errors.
 */
int download_fullfiles(struct list *files, int *num_downloads)
{
	void *download_handle;

	if (!files) {
		return SWUPD_OK;
	}

	download_handle = swupd_curl_parallel_download_start(get_max_xfer(MAX_XFER));
	swupd_curl_parallel_download_set_callbacks(download_handle, download_successful, download_error, NULL);
	info("Starting download of remaining update content. This may take a while...\n");
	if (!download_handle) {
		/* If we hit this point, the network is accessible but we were
		 * unable to download the needed files. This is a terminal error
		 * and we need good logging */
		return -SWUPD_COULDNT_DOWNLOAD_FILE;
	}

	return download_loop(download_handle, files, num_downloads);
}
