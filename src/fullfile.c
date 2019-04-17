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
		warn("Failed to copy local mix file: %s\n", file->staging);
	}

	free_string(&url);
	free_string(&filename);
}

static void download_file(void *download_handle, struct file *file)
{
	char *url, *filename;

	string_or_die(&filename, "%s/download/.%s.tar", state_dir, file->hash);
	string_or_die(&url, "%s/%i/files/%s.tar", content_url, file->last_change, file->hash);
	swupd_curl_parallel_download_enqueue(download_handle, url, filename, file->hash, file);
	free_string(&url);
	free_string(&filename);
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
		warn("Error for %s tarfile extraction, (check free space for %s?)\n",
		     ((struct file *)data)->hash, state_dir);
	}
	return true;
}

static double fullfile_query_total_download_size(struct list *files)
{
	double size = 0;
	double total_size = 0;
	struct file *file = NULL;
	struct list *list = NULL;
	char *url = NULL;
	int count = 0;

	for (list = list_head(files); list; list = list->next) {
		file = list->data;

		/* if it is a file from a mix, we won't download it */
		if (file->is_mix) {
			continue;
		}

		string_or_die(&url, "%s/%i/files/%s.tar", content_url, file->last_change, file->hash);
		size = swupd_curl_query_content_size(url);
		if (size != -1) {
			total_size += size;
		} else {
			debug("The header for file %s could not be downloaded\n", file->filename);
			free_string(&url);
			return -SWUPD_COULDNT_DOWNLOAD_FILE;
		}

		count++;
		debug("File: %s (%.2lf Mb)\n", url, size / 1000000);
		free_string(&url);
	}

	debug("Number of files to download: %d\n", count);
	debug("Total size of files to be downloaded: %.2lf Mb\n", total_size / 1000000);
	return total_size;
}

/*
 * Download fullfiles from the list of files.
 *
 * Return 0 on success or a negative number or errors.
 */
int download_fullfiles(struct list *files, int *num_downloads)
{
	void *download_handle;
	struct list *iter;
	struct list *need_download = NULL;
	struct file *file;
	struct stat stat;
	struct download_progress download_progress = { 0, 0, 0 };
	unsigned int complete = 0;
	unsigned int list_length;
	const unsigned int MAX_FILES = 1000;

	if (!files) {
		/* nothing needs to be downloaded */
		return SWUPD_OK;
	}

	/* make a new list with only the files we actually need to download */
	for (iter = list_head(files); iter; iter = iter->next) {
		char *targetfile;
		file = iter->data;

		if (file->is_deleted || file->do_not_update) {
			continue;
		}

		string_or_die(&targetfile, "%s/staged/%s", state_dir, file->hash);
		if (lstat(targetfile, &stat) != 0 || !verify_file(file, targetfile)) {
			need_download = list_append_data(need_download, file);
		}

		free_string(&targetfile);
	}

	if (!need_download) {
		/* no file needs to be downloaded */
		info("No extra files need to be downloaded\n");
		progress_complete_step();
		return 0;
	}

	/* we need to download some files, so set up curl */
	download_handle = swupd_curl_parallel_download_start(get_max_xfer(MAX_XFER));
	swupd_curl_parallel_download_set_callbacks(download_handle, download_successful, download_error, NULL);
	if (!download_handle) {
		/* If we hit this point, the network is accessible but we were
		 * unable to download the needed files. This is a terminal error
		 * and we need good logging */
		return -SWUPD_COULDNT_DOWNLOAD_FILE;
	}

	/* getting the size of many files can be very expensive, so if
	 * the files are not too many, get their size, otherwise just use their count
	 * to report progress */
	list_length = list_len(need_download);
	if (list_length < MAX_FILES) {
		download_progress.total_download_size = fullfile_query_total_download_size(need_download);
		if (download_progress.total_download_size > 0) {
			/* enable the progress callback */
			swupd_curl_parallel_download_set_progress_callbacks(download_handle, swupd_progress_callback, &download_progress);
		} else {
			debug("Couldn't get the size of the files to download, using number of files instead\n");
			download_progress.total_download_size = 0;
		}
	} else {
		debug("Too many files to calculate download size (%d files), maximum is %d. Using number of files instead\n", list_length, MAX_FILES);
	}

	/* download loop */
	info("Starting download of remaining update content. This may take a while...\n");

	for (iter = list_head(need_download); iter; iter = iter->next) {
		file = iter->data;

		if (file->is_mix) {
			download_mix_file(file);
		} else {
			download_file(download_handle, file);
		}

		/* fall back for progress reporting when the download size
		* could not be determined */
		if (download_progress.total_download_size == 0) {
			complete++;
			progress_report(complete, list_length);
		}
	}
	info("\n");
	list_free_list(need_download);

	return swupd_curl_parallel_download_end(download_handle, num_downloads);
}
