/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2020 Intel Corporation.
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

#include <sys/stat.h>
#include <unistd.h>

#include "swupd.h"

/* hysteresis thresholds */
//FIXME #562
#define MAX_XFER 15

static int download_file(struct swupd_curl_parallel_handle *download_handle, struct file *file)
{
	int ret = -1;
	char *url, *filename;

	filename = statedir_get_fullfile_tar(file->hash);
	string_or_die(&url, "%s/%i/files/%s.tar", globals.content_url, file->last_change, file->hash);
	ret = swupd_curl_parallel_download_enqueue(download_handle, url, filename, file->hash, file);
	FREE(url);
	FREE(filename);

	return ret;
}

static bool download_successful(void *data)
{
	if (!data) {
		return false;
	}

	if (untar_full_download(data) != 0) {
		warn("Error for %s tarfile extraction, (check free space for %s?)\n",
		     ((struct file *)data)->hash, globals.cache_dir);
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

		string_or_die(&url, "%s/%i/files/%s.tar", globals.content_url, file->last_change, file->hash);
		size = swupd_curl_query_content_size(url);
		if (size > 0) {
			total_size += size;
		} else {
			debug("The header for file %s could not be downloaded\n", file->filename);
			FREE(url);
			return -SWUPD_COULDNT_DOWNLOAD_FILE;
		}

		count++;
		debug("File: %s (%.2lf MB)\n", url, size / 1000000);
		FREE(url);
	}

	debug("Number of files to download: %d\n", count);
	debug("Total size of files to be downloaded: %.2lf MB\n", total_size / 1000000);
	return total_size;
}

static int get_cached_fullfile(struct file *file)
{
	char *targetfile;
	char *targetfile_cache;
	struct stat stat;
	int ret = 0;

	/* Check the statedir and statedir-cache for the expected fullfile. When the
	 * fullfile exists in the statedir-cache, it is copied to the statedir. */
	targetfile = statedir_get_staged_file(file->hash);
	if (lstat(targetfile, &stat) != 0 || !verify_file(file, targetfile)) {
		ret = 1;

		if (globals.state_dir_cache != NULL) {
			targetfile_cache = statedir_dup_get_staged_file(file->hash);
			if (lstat(targetfile_cache, &stat) == 0 && verify_file(file, targetfile_cache)) {
				if (link_or_copy_all(targetfile_cache, targetfile) == 0) {
					ret = 0;
				}
			}
			FREE(targetfile_cache);
		}
	}
	FREE(targetfile);

	return ret;
}

/*
 * Download fullfiles from the list of files.
 *
 * Return 0 on success or a negative number or errors.
 */
int download_fullfiles(struct list *files, int *num_downloads)
{
	int ret = 0;
	struct swupd_curl_parallel_handle *download_handle;
	struct list *iter;
	struct list *need_download = NULL;
	struct file *file;
	struct download_progress download_progress = { 0, 0 };
	int complete = 0;
	int list_length;
	const int MAX_FILES = 1000;

	progress_next_step("validate_fullfiles", PROGRESS_BAR);
	info("Validate downloaded files\n");
	if (!files) {
		/* nothing needs to be downloaded */
		progress_next_step("download_fullfiles", PROGRESS_BAR);
		goto out;
	}

	files = list_head(files);
	list_length = list_len(files);
	/* make a new list with only the files we actually need to download */
	for (iter = files; iter; iter = iter->next) {
		file = iter->data;
		progress_report(complete++, list_length);

		if (file->is_deleted || file->do_not_update) {
			continue;
		}

		/* download the fullfile when it is missing from the statedir and statedir-cache */
		if (get_cached_fullfile(file) != 0) {
			need_download = list_append_data(need_download, file);
		}
	}

	progress_next_step("download_fullfiles", PROGRESS_BAR);
	if (!need_download) {
		/* no file needs to be downloaded */
		info("No extra files need to be downloaded\n");
		goto out;
	}

	/* we need to download some files, so set up curl */
	download_handle = swupd_curl_parallel_download_start(get_max_xfer(MAX_XFER));
	swupd_curl_parallel_download_set_callbacks(download_handle, download_successful, NULL, NULL);
	if (!download_handle) {
		/* If we hit this point, the network is accessible but we were
		 * unable to download the needed files. This is a terminal error
		 * and we need good logging */
		list_free_list(need_download);
		ret = -SWUPD_COULDNT_DOWNLOAD_FILE;
		goto out;
	}

	/* getting the size of many files can be very expensive, so if
	 * the files are not too many, get their size, otherwise just use their count
	 * to report progress */
	list_length = list_len(need_download);
	complete = 0;
	if (list_length < MAX_FILES) {
		/* remove tar duplicates from the list first */
		need_download = list_sort(need_download, cmp_file_hash);

		/* different directories may need the same tar, in those cases it needs
		 * to be downloaded only once, the tar for each file is downloaded from
		 * <last_change>/files/<hash>.tar */
		need_download = list_sorted_deduplicate(need_download, cmp_file_hash_last_change, NULL);

		download_progress.total_download_size = fullfile_query_total_download_size(need_download);
		if (download_progress.total_download_size > 0) {
			/* enable the progress callback */
			swupd_curl_parallel_download_set_progress_callback(download_handle, swupd_progress_callback, &download_progress);
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

		ret = download_file(download_handle, file);

		/* fall back for progress reporting when the download size
		 * could not be determined */
		if (download_progress.total_download_size == 0) {
			complete++;
			progress_report(complete, list_length);
		}

		if (ret < 0) {
			swupd_curl_parallel_download_cancel(download_handle);
			list_free_list(need_download);
			goto out;
		}
	}
	list_free_list(need_download);

	progress_next_step("extract_fullfiles", PROGRESS_UNDEFINED);
	return swupd_curl_parallel_download_end(download_handle, num_downloads);

out:
	progress_next_step("extract_fullfiles", PROGRESS_UNDEFINED);
	return ret;
}
