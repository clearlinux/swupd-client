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
#include <curl/curl.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "hashmap.h"
#include "swupd-build-variant.h"
#include "swupd.h"

/* This file provides a managed download facility for code that needs a set of
 * update tar files.  Such code starts with one call to "start_full_download()",
 * then makes a series of calls to "full_download()" once per desired file, and
 * then finishes with one call to "end_full_download()" which will block until
 * the previously queued downloads are completed and and untarred.
 */

static CURLM *mcurl = NULL;
static size_t mcurl_size = 0;
static struct list *failed = NULL;

/*
 * The following represents a hash data structure used inside download.c
 * to track file downloads asynchonously via libcurl.  In the past a
 * single linked list was used, but we may have a very large number of
 * files and repeatedly scan the list could become expensive.  The hashmap
 * gives info on what HASH.tar files will be downloaded.  A de-duplicated
 * set of files is downloaded.  The downloads must complete prior to
 * traversing a manifest, doing hash comparisons, and (re)staging any files
 * whose hash miscompares.
 *
 * file->hash[0:1] acts as index into the arrays
 */
#define SWUPD_CURL_HASH_BUCKETS 256

/*
 * Converts a character to its hexadecimal value (0 to 15 assuming that the
 * input was from the set 0123456789abcdef).
 */
#define HASH_VALUE(h) (h >= 'a' ? h - 'a' + 10 : h - '0')

/*
 * converts two characters to a value in the range 0 to 255.
 */
#define HASH_TO_KEY(hash) (HASH_VALUE(hash[0]) << 4 | HASH_VALUE(hash[1]))

static struct hashmap *swupd_curl_hashmap = NULL;

/* try to insert the file into the hashmap download queue
 * returns 1 if no download is needed
 * returns 0 if download is needed
 * returns -1 if error */
static int swupd_curl_hashmap_insert(struct file *file)
{
	char *tar_dotfile;
	char *targetfile;
	struct stat stat;

	if (!hashmap_put(swupd_curl_hashmap, file)) {
		// hash already in download queue
		return 1;
	}

	// if valid target file is already here, no need to download
	string_or_die(&targetfile, "%s/staged/%s", state_dir, file->hash);

	if (lstat(targetfile, &stat) == 0) {
		/* file exists */
		if (verify_file(file, targetfile)) {
			/* hash matches, no download necessary */
			free_string(&targetfile);
			return 1;
		} else {
			/* hash mismatch, remove the staged file to enable re-download */
			unlink(targetfile);
		}
	}

	free_string(&targetfile);

	// hash not in queue and not present in staged

	// clean up in case any prior download failed in a partial state
	string_or_die(&tar_dotfile, "%s/download/.%s.tar", state_dir, file->hash);
	unlink(tar_dotfile);
	free_string(&tar_dotfile);

	return 0;
}

/* hysteresis thresholds */
static size_t MAX_XFER = 25;
static size_t MAX_XFER_BOTTOM = 15;

static bool file_hash_cmp(const void *a, const void *b)
{
	const struct file *fa = a;
	const struct file *fb = b;

	return hash_equal(fa->hash, fb->hash);
}

static size_t file_hash_value(const void *data)
{
	return HASH_VALUE(((struct file *)data)->hash[0]);
}

int start_full_download(void)
{
	if (swupd_curl_hashmap) {
		return -1;
	}

	failed = NULL;
	swupd_curl_hashmap = hashmap_new(SWUPD_CURL_HASH_BUCKETS, file_hash_cmp, file_hash_value);

	mcurl = curl_multi_init();
	if (mcurl == NULL) {
		return -1;
	}

	curl_multi_setopt(mcurl, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX | CURLPIPE_HTTP1);

	fprintf(stderr, "Starting download of remaining update content. This may take a while...\n");

	return 0;
}

static void free_curl_list_data(void *data)
{
	struct file *file = (struct file *)data;
	CURL *curl = file->curl;
	(void)swupd_download_file_complete(CURLE_OK, file);
	if (curl != NULL) {
		/* Must remove handle out of multi queue first!*/
		curl_multi_remove_handle(mcurl, curl);
		curl_easy_cleanup(curl);
		file->curl = NULL;
	}
}

/* list the tarfile content, and verify it contains only one line equal to the expected hash.
 * loop through all the content to detect the case where archive contains more than one file.
 */
static int check_tarfile_content(struct file *file, const char *tarfilename)
{
	int err;
	char *tarcommand;
	FILE *tar;
	int count = 0;

	string_or_die(&tarcommand, TAR_COMMAND " -tf %s/download/%s.tar 2> /dev/null", state_dir, file->hash);

	err = access(tarfilename, R_OK);
	if (err) {
		goto free_tarcommand;
	}

	tar = popen(tarcommand, "r");
	if (tar == NULL) {
		err = -1;
		goto free_tarcommand;
	}

	while (!feof(tar)) {
		char *c;
		char buffer[PATH_MAXLEN];

		if (fgets(buffer, PATH_MAXLEN, tar) == NULL) {
			if (count != 1) {
				err = -1;
			}
			break;
		}

		c = strchr(buffer, '\n');
		if (c) {
			*c = 0;
		}
		if (c && (c != buffer) && (*(c - 1) == '/')) {
			/* strip trailing '/' from directory tar */
			*(c - 1) = 0;
		}
		if (strcmp(buffer, file->hash) != 0) {
			err = -1;
			break;
		}
		count++;
	}

	pclose(tar);
free_tarcommand:
	free_string(&tarcommand);

	return err;
}

/* This function will break if the same HASH.tar full file is downloaded
 * multiple times in parallel. */
int untar_full_download(void *data)
{
	struct file *file = data;
	char *tarfile;
	char *tar_dotfile;
	char *targetfile;
	struct stat stat;
	int err;

	string_or_die(&tar_dotfile, "%s/download/.%s.tar", state_dir, file->hash);
	string_or_die(&tarfile, "%s/download/%s.tar", state_dir, file->hash);
	string_or_die(&targetfile, "%s/staged/%s", state_dir, file->hash);

	/* If valid target file already exists, we're done.
	 * NOTE: this should NEVER happen given the checking that happens
	 *       ahead of queueing a download.  But... */
	if (lstat(targetfile, &stat) == 0) {
		if (verify_file(file, targetfile)) {
			unlink(tar_dotfile);
			unlink(tarfile);
			free_string(&tar_dotfile);
			free_string(&tarfile);
			free_string(&targetfile);
			return 0;
		} else {
			unlink(tarfile);
			unlink(targetfile);
		}
	} else if (lstat(tarfile, &stat) == 0) {
		/* remove tar file from possible past failure */
		unlink(tarfile);
	}

	err = rename(tar_dotfile, tarfile);
	if (err) {
		free_string(&tar_dotfile);
		goto exit;
	}
	free_string(&tar_dotfile);

	err = check_tarfile_content(file, tarfile);
	if (err) {
		goto exit;
	}

	/* modern tar will automatically determine the compression type used */
	char *outputdir;
	string_or_die(&outputdir, "%s/staged", state_dir);
	err = extract_to(tarfile, outputdir);
	free_string(&outputdir);
	if (err) {
		fprintf(stderr, "ignoring tar extract failure for fullfile %s.tar (ret %d)\n",
			file->hash, err);
		goto exit;
		/* TODO: respond to ARCHIVE_RETRY error codes
		 * libarchive returns ARCHIVE_RETRY when tar extraction fails but the
		 * operation is retry-able. We need to determine if it is worth our time
		 * to retry in these situations. */
	} else {
		/* Only unlink when tar succeeded, so we can examine the tar file
		 * in the failure case. */
		unlink(tarfile);
	}

	err = lstat(targetfile, &stat);
	if (!err && !verify_file(file, targetfile)) {
		/* Download was successful but the hash was bad. This is fatal*/
		fprintf(stderr, "Error: File content hash mismatch for %s (bad server data?)\n", targetfile);
		exit(EXIT_FAILURE);
	}

exit:
	free_string(&tarfile);
	free_string(&targetfile);
	if (err) {
		unlink_all_staged_content(file);
	}
	return err;
}

/* Try to process at most COUNT messages from the curl multi-stack, and enforce
 * the hysteresis when BOUNDED is true. The COUNT value is a best guess about
 * the number of messages ready to process, because it may include file
 * descriptors internal to libcurl as well (see curl_multi_wait(3)). */
static int perform_curl_io_and_complete(int count, bool bounded)
{
	CURLMsg *msg;
	long ret;
	CURLcode curl_ret;

	while (count > 0) {
		CURL *handle;
		struct file *file;
		char *url = NULL;
		bool local_download;

		/* This function may return NULL before processing the
		 * requested number of messages (stored in variable "count").
		 * The second argument here (*msgs_in_queue) could be used to
		 * update "count", but if its value is greater than "count",
		 * more messages would be processed than desired. Note:
		 * curl_multi_info_read() does not accept NULL for the second
		 * argument to indicate an unused value. */
		int unused;
		msg = curl_multi_info_read(mcurl, &unused);
		if (!msg) {
			/* Either there were fewer than COUNT messages to
			 * process, or the multi-stack is now empty. */
			break;
		}
		if (msg->msg != CURLMSG_DONE) {
			continue;
		}

		handle = msg->easy_handle;
		ret = 404;
		curl_ret = curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &ret);
		if (curl_ret != CURLE_OK) {
			continue;
		}

		curl_ret = curl_easy_getinfo(handle, CURLINFO_PRIVATE, (char **)&file);
		if (curl_ret != CURLE_OK) {
			curl_easy_cleanup(handle);
			continue;
		}

		curl_ret = curl_easy_getinfo(handle, CURLINFO_EFFECTIVE_URL, &url);
		if (curl_ret != CURLE_OK) {
			curl_easy_cleanup(handle);
			continue;
		}
		local_download = strncmp(url, "file://", 7) == 0;

		/* Get error code from easy handle and augment it if
		 * completing the download encounters further problems. */
		curl_ret = msg->data.result;
		curl_ret = swupd_download_file_complete(curl_ret, file);

		/* The easy handle may have an error set, even if the server returns
		 * HTTP 200, so retry the download for this case. */
		if (ret == 200 && curl_ret != CURLE_OK) {
			fprintf(stderr, "Error for %s download: %s\n", file->hash,
				curl_easy_strerror(msg->data.result));
			failed = list_prepend_data(failed, file);
		} else if (ret == 200) {
			/* When both web server and CURL report success, only then
			 * proceed to uncompress. */
			if (untar_full_download(file)) {
				fprintf(stderr, "Error for %s tarfile extraction, (check free space for %s?)\n",
					file->hash, state_dir);
				failed = list_prepend_data(failed, file);
			}
		} else if (ret == 0) {
			/* When using the FILE:// protocol, 0 indicates success.
			 * Otherwise, it means the web server hasn't responded yet.
			 */
			if (local_download) {
				if (untar_full_download(file)) {
					fprintf(stderr, "Error for %s tarfile extraction, (check free space for %s?)\n",
						file->hash, state_dir);
					failed = list_prepend_data(failed, file);
				}
			} else {
				fprintf(stderr, "Error for %s download: No response received\n",
					file->hash);
				failed = list_prepend_data(failed, file);
			}
		} else {
			fprintf(stderr, "Error for %s download: Received %ld response\n", file->hash, ret);
			failed = list_prepend_data(failed, file);

			unlink_all_staged_content(file);
		}
		free_string(&file->staging);

		/* NOTE: Intentionally no removal of file from hashmap.  All
		 * needed files need determined and queued in one complete
		 * preparation phase.  Once all needed files are all present,
		 * they can be staged.  Otherwise a complex datastructure and
		 * retries are needed to insure only one download of a file
		 * happens fully to success AND a HASH.tar is uncompressed to
		 * and HASH and staged to the _multiple_ filenames with that
		 * hash. */

		curl_multi_remove_handle(mcurl, handle);
		curl_easy_cleanup(handle);
		file->curl = NULL;

		/* "bounded" is false when the remainder of the multi-stack is
		 * to be processed, ignoring the hysteresis bound. */
		if (bounded) {
			count--;
		}

		/* A response has been completely processed by this point, so
		 * make the stack shorter. */
		mcurl_size--;
	}

	return 0;
}

/* Fixed window added to hysteresis upper bound before it is enforced. */
#define BUFFER 5

/* 2 limits, so that we can have hysteresis in behavior. We let the caller
 * add new transfer up until the queue reaches the high threshold. At this point
 * we don't return to the caller and instead process the queue until its len
 * gets below the low threshold */
static int poll_fewer_than(size_t xfer_queue_high, size_t xfer_queue_low)
{
	CURLMcode curlm_ret;
	int running;

	curlm_ret = curl_multi_perform(mcurl, &running);
	if (curlm_ret != CURLM_OK) {
		return -1;
	}

	// There are not enough transfers in queue yet
	if (mcurl_size <= xfer_queue_high) {
		return 0;
	}

	/* When the high bound for the hysteresis is reached, process events
	 * until the low bound is reached */
	while (mcurl_size > xfer_queue_low) {
		int numfds = 0;

		// Wait for activity on the multi-stack
		curlm_ret = curl_multi_wait(mcurl, NULL, 0, 500, &numfds);
		if (curlm_ret != CURLM_OK) {
			return -1;
		}

		/* Either a timeout was hit, or no events to process.
		 * Note: curl_multi_wait() never sets numfds < 0 */
		if (numfds == 0) {
			curlm_ret = curl_multi_perform(mcurl, &running);
			if (curlm_ret != CURLM_OK) {
				return -1;
			}

			/* If there are no longer any transfers in progress,
			 * there is no work left to do. */
			if (running == 0) {
				break;
			}
		}

		// Do more work before processing the queue
		curlm_ret = curl_multi_perform(mcurl, &running);
		if (curlm_ret != CURLM_OK) {
			return -1;
		}

		// Instead of using "numfds" as a hint for how many transfers
		// to process, try to drain the queue to the lower bound.
		int remaining = mcurl_size - xfer_queue_low;

		if (perform_curl_io_and_complete(remaining, true) != 0) {
			return -1;
		}

		// Do more work, in preparation for the next curl_multi_wait()
		curlm_ret = curl_multi_perform(mcurl, &running);
		if (curlm_ret != CURLM_OK) {
			return -1;
		}
	}

	// Avoid escaping the hysteresis upper bound; the expected case is for
	// the stack size to be xfer_queue_high + 1 before entering the below
	// while loop. If the stack attempts to grow larger than this, it means
	// that curl_multi_perform() indicated no transfers in progress more
	// than once after curl_multi_wait() reported no file descriptors with
	// activity. Meanwhile, there may be any number of future transfers
	// attempted, with curl returning no progress each time, resulting in
	// mcurl_size still being too large. If this condition occurs
	// (exceeding xfer_queue_high + 1), do not return an error unless no
	// activity is reported the next BUFFER times. When an error is
	// returned, the most recent attempted transfer is added to the failed
	// list, later to be retried.
	if (xfer_queue_low != xfer_queue_high && mcurl_size > xfer_queue_high + 1 + BUFFER) {
		return -1;
	}

	return 0;
}

extern int nonpack;

/* full_download() attempts to enqueue a file for later asynchronous download
   - NOTE: See swupd_curl_get_file() for single file synchronous downloads. */
void full_download(struct file *file)
{
	char *url = NULL;
	CURL *curl = NULL;
	int ret = -EFULLDOWNLOAD;
	char *filename = NULL;
	CURLMcode curlm_ret = CURLM_OK;
	CURLcode curl_ret = CURLE_OK;

	file->fh = NULL;
	ret = swupd_curl_hashmap_insert(file);
	if (ret > 0) { /* no download needed */
		/* File already exists - report success */
		ret = 0;
		goto out_good;
	} else if (ret < 0) { /* error */
		goto out_bad;
	} /* else (ret == 0)	   download needed */

	/* If we get here the pack is missing a file so we have to download it */
	nonpack++;

	curl = curl_easy_init();
	if (curl == NULL) {
		goto out_bad;
	}
	file->curl = curl;

	ret = poll_fewer_than(MAX_XFER, MAX_XFER_BOTTOM);
	if (ret != 0) {
		goto out_bad;
	}

	string_or_die(&url, "%s/%i/files/%s.tar", content_url, file->last_change, file->hash);

	string_or_die(&filename, "%s/download/.%s.tar", state_dir, file->hash);
	file->staging = filename;

	curl_ret = curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	if (curl_ret != CURLE_OK) {
		goto out_bad;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_PRIVATE, (void *)file);
	if (curl_ret != CURLE_OK) {
		goto out_bad;
	}
	curl_ret = swupd_download_file_start(file);
	if (curl_ret != CURLE_OK) {
		goto out_bad;
	}
	curl_ret = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)file->fh);
	if (curl_ret != CURLE_OK) {
		goto out_bad;
	}

	curl_ret = swupd_curl_set_basic_options(curl, url);
	if (curl_ret != CURLE_OK) {
		goto out_bad;
	}

	curlm_ret = curl_multi_add_handle(mcurl, curl);
	if (curlm_ret != CURLM_OK) {
		goto out_bad;
	}

	/* The current multi-stack size is not exposed by libcurl, so we track
	 * it with a counter variable. */
	mcurl_size++;

	ret = poll_fewer_than(MAX_XFER + 10, MAX_XFER);
	if (ret != 0) {
		goto out_bad;
	}

	ret = 0;
	goto out_good;

out_bad:
	failed = list_prepend_data(failed, file);
	free_curl_list_data(file);
	free_string(&filename);
out_good:
	free_string(&url);
}

struct list *end_full_download(void)
{
	fprintf(stderr, "Finishing download of update content...\n");

	if (poll_fewer_than(0, 0) == 0) {
		// The multi-stack is now emptied.
		perform_curl_io_and_complete(1, false);
	}

	hashmap_free_hash_and_data(swupd_curl_hashmap, free_curl_list_data);
	swupd_curl_hashmap = NULL;

	curl_multi_cleanup(mcurl);
	return failed;
}
