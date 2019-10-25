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

static int compare_fullfile(const void *a, const void *b)
{
	struct file *file1 = (struct file *)a;
	struct file *file2 = (struct file *)b;
	int comp;

	comp = strcmp(file1->hash, file2->hash);
	if (comp != 0) {
		return comp;
	}

	/* they have the same hash, now let's check the version */
	return file1->last_change - file2->last_change;
}

static void download_mix_file(struct file *file)
{
	char *url, *filename;

	string_or_die(&url, "%s/%i/files/%s.tar", MIX_STATE_DIR, file->last_change, file->hash);
	string_or_die(&filename, "%s/download/.%s.tar", globals.state_dir, file->hash);

	/* Mix content is local, so don't queue files up for curl downloads */
	if (link_or_rename(url, filename) == 0) {
		untar_full_download(file);
	} else {
		warn("Failed to copy local mix file: %s\n", file->staging);
	}

	free_string(&url);
	free_string(&filename);
}

static void download_file(struct swupd_curl_parallel_handle *download_handle, struct file *file)
{
	char *url, *filename;

	string_or_die(&filename, "%s/download/.%s.tar", globals.state_dir, file->hash);
	string_or_die(&url, "%s/%i/files/%s.tar", globals.content_url, file->last_change, file->hash);
	swupd_curl_parallel_download_enqueue(download_handle, url, filename, file->hash, file);
	free_string(&url);
	free_string(&filename);
}

static bool download_successful(void *data)
{
	if (!data) {
		return false;
	}

	if (untar_full_download(data) != 0) {
		warn("Error for %s tarfile extraction, (check free space for %s?)\n",
		     ((struct file *)data)->hash, globals.state_dir);
	}
	return true;
}

static double fullfile_query_total_download_size(struct list *files)
{
	long size = 0;
	long total_size = 0;
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

		string_or_die(&url, "%s/%i/files/%s.tar", globals.content_url, file->last_change, file->hash);
		size = swupd_curl_query_content_size(url);
		if (size != -1) {
			total_size += size;
		} else {
			debug("The header for file %s could not be downloaded\n", file->filename);
			free_string(&url);
			return -SWUPD_COULDNT_DOWNLOAD_FILE;
		}

		count++;
		debug("File: %s (%.2lf MB)\n", url, (double)size / 1000000);
		free_string(&url);
	}

	debug("Number of files to download: %d\n", count);
	debug("Total size of files to be downloaded: %.2lf MB\n", (double)total_size / 1000000);
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
	string_or_die(&targetfile, "%s/staged/%s", globals.state_dir, file->hash);
	if (lstat(targetfile, &stat) != 0 || !verify_file(file, targetfile)) {
		ret = 1;

		if (globals.state_dir_cache != NULL) {
			string_or_die(&targetfile_cache, "%s/staged/%s", globals.state_dir_cache, file->hash);
			if (lstat(targetfile_cache, &stat) == 0 && verify_file(file, targetfile_cache)) {
				if (link_or_copy_all(targetfile_cache, targetfile) == 0) {
					ret = 0;
				}
			}
			free_string(&targetfile_cache);
		}
	}
	free_string(&targetfile);

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
		need_download = list_sort(need_download, file_sort_hash);

		/* different directories may need the same tar, in those cases it needs
		 * to be downloaded only once, the tar for each file is downloaded from
		 * <last_change>/files/<hash>.tar */
		need_download = list_deduplicate(need_download, compare_fullfile, NULL);

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
	list_free_list(need_download);

	progress_next_step("extract_fullfiles", PROGRESS_UNDEFINED);
	return swupd_curl_parallel_download_end(download_handle, num_downloads);

out:
	progress_next_step("extract_fullfiles", PROGRESS_UNDEFINED);
	return ret;
}
