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

#include "lib/hashmap.h"
#include "lib/thread_pool.h"
#include "swupd.h"
#include "swupd_curl_internal.h"

// Wait time when curl has no fd in use in us (100ms)
#define CURL_NOFD_WAIT 100000
// Curl timeout in ms
#define CURL_MULTI_TIMEOUT 500
// Fixed window added to hysteresis upper bound before it is enforced.
#define XFER_QUEUE_BUFFER 5

/*
 * This file provides a managed download facility for parallelizing download
 * files. To use this functionality, create a handler using
 * swupd_curl_parallel_download_start(), enqueue files using
 * swupd_curl_parallel_download_enqueue() and finish the process using
 * swupd_curl_parallel_download_end(), which will block until the previously
 * queued downloads are completed.
 */

/*
 * The following represents a hash data structure used inside download.c
 * to track file downloads in parallel via libcurl. In the past a
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

struct swupd_curl_parallel_handle {
	int retry_delay;	     /* Retry delay */
	size_t mcurl_size, max_xfer; /* hysteresis parameters */
	bool resume_failed;
	int last_retry; /* keep the largest retry number so far */

	CURLM *mcurl;			    /* Curl handle */
	struct list *failed;		    /* List of failed downloads */
	struct hashmap *curl_hashmap;	    /* Hashmap mentioned above */
	struct tp *thpool;		    /* Pointer to the threadpool */
	swupd_curl_success_cb success_cb;   /* Callback to success function */
	swupd_curl_error_cb error_cb;	    /* Callback to error function */
	swupd_curl_free_cb free_cb;	    /* Callback to free user data */
	swupd_curl_progress_cb progress_cb; /* Callback to report download progress */
	void *data;
};

/*
 * Struct represinting a single file being downloaded.
 */
struct multi_curl_file {
	struct curl_file file;		/* Curl file information */
	enum download_status status;	/* status of last download try */
	char retries;			/* Number of retried performed so far */
	CURL *curl;			/* curl handle if downloading */
	char *url;			/* The url to be downloaded from */
	size_t hash_key;		/* hash_key of this file */
	const char *hash;		/* Unique identifier of this file. */
	swupd_curl_success_cb callback; /* Holds original success callback to be wrapped */

	void *data;	/* user's data */
	bool cb_retval; /* return value from callback */
	struct file_progress *progress;
};

/*
 * Wrap the success callback for each thread.
 * Return values are added to multi_curl_file's cb_retval
 */
static void success_callback_wrapper(void *data)
{
	struct multi_curl_file *file = data;
	file->cb_retval = file->callback(file->data);
}

static bool file_hash_cmp(const void *a, const void *b)
{
	const struct multi_curl_file *fa = a;
	const struct multi_curl_file *fb = b;

	if (!fa->hash || !fb->hash) {
		return str_cmp(fa->file.path, fb->file.path) == 0;
	}
	return hash_equal(fa->hash, fb->hash);
}

static size_t file_hash_value(const void *data)
{
	return ((struct multi_curl_file *)data)->hash_key;
}

static void free_curl_file(struct swupd_curl_parallel_handle *h, struct multi_curl_file *file)
{
	CURL *curl = file->curl;
	swupd_download_file_close(CURLE_OK, &file->file);

	if (h->free_cb) {
		h->free_cb(file->data);
	}

	free_and_clear_pointer(&file->url);
	free_and_clear_pointer(&file->file.path);
	if (curl != NULL) {
		/* Must remove handle out of multi queue first!*/
		curl_multi_remove_handle(h->mcurl, curl);
		curl_easy_cleanup(curl);
		file->curl = NULL;
	}

	free(file->progress);
	free(file);
}

static void reevaluate_number_of_parallel_downloads(struct swupd_curl_parallel_handle *h, int retry)
{
	if (h->last_retry >= retry || h->max_xfer == 1) {
		return;
	}

	h->last_retry = retry;

	// It's not expected to have any retry if the connection is good
	// So stop downloading in parallel if the connection is bad
	h->max_xfer = 1;

	info("Curl - Reducing number of parallel downloads to %ld\n", h->max_xfer);
}

static bool swupd_async_curl_init()
{
	// <0 non-initialized
	// 0 success
	// >0 error
	static int initialized = -1;

	if (initialized >= 0) {
		return initialized == 0;
	}

	initialized = -check_connection(globals.content_url);
	return initialized == 0;
}

struct swupd_curl_parallel_handle *swupd_curl_parallel_download_start(size_t max_xfer)
{
	struct swupd_curl_parallel_handle *h;

	if (!swupd_async_curl_init()) {
		return NULL;
	}

	h = calloc(1, sizeof(struct swupd_curl_parallel_handle));
	ON_NULL_ABORT(h);

	h->mcurl = curl_multi_init();
	if (h->mcurl == NULL) {
		goto error;
	}

	/* Libarchive is not thread safe when using the archive_disk_write api.
	 * So for now, use only 1 thread to handle untar/extraction */
	h->thpool = tp_start(1);
	if (!h->thpool) {
		h->thpool = tp_start(0);
		if (!h->thpool) {
			error("Unable to create a thread pool");
			goto error;
		}

		warn("Curl - Unable to create a thread pool - downloading files synchronously\n");
		max_xfer = 0;
	}

	curl_multi_setopt(h->mcurl, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX | CURLPIPE_HTTP1);

	h->max_xfer = max_xfer;
	h->curl_hashmap = hashmap_new(SWUPD_CURL_HASH_BUCKETS, file_hash_cmp, file_hash_value);
	h->retry_delay = globals.retry_delay;

	return h;
error:
	free(h);
	return NULL;
}

void swupd_curl_parallel_download_set_callbacks(struct swupd_curl_parallel_handle *h, swupd_curl_success_cb success_cb, swupd_curl_error_cb error_cb, swupd_curl_free_cb free_cb)
{
	if (!h) {
		error("Curl - Invalid parallel download handle\n");
		return;
	}

	h->success_cb = success_cb;
	h->error_cb = error_cb;
	h->free_cb = free_cb;
}

void swupd_curl_parallel_download_set_progress_callback(struct swupd_curl_parallel_handle *h, swupd_curl_progress_cb progress_cb, void *data)
{
	if (!h) {
		error("Curl - Invalid parallel download handle\n");
		return;
	}

	h->progress_cb = progress_cb;
	h->data = data;
}

// Try to process at most COUNT messages from the curl multi-stack.
static int perform_curl_io_and_complete(struct swupd_curl_parallel_handle *h, int count)
{
	CURLMsg *msg;
	CURLcode curl_ret;

	while (count > 0) {
		CURL *handle;
		struct multi_curl_file *file;

		/* This function may return NULL before processing the
		 * requested number of messages (stored in variable "count").
		 * The second argument here (*msgs_in_queue) could be used to
		 * update "count", but if its value is greater than "count",
		 * more messages would be processed than desired. Note:
		 * curl_multi_info_read() does not accept NULL for the second
		 * argument to indicate an unused value. */
		int unused;
		msg = curl_multi_info_read(h->mcurl, &unused);
		if (!msg) {
			/* Either there were fewer than COUNT messages to
			 * process, or the multi-stack is now empty. */
			break;
		}
		if (msg->msg != CURLMSG_DONE) {
			continue;
		}

		handle = msg->easy_handle;

		curl_ret = curl_easy_getinfo(handle, CURLINFO_PRIVATE, (char **)&file);
		if (curl_ret != CURLE_OK) {
			curl_easy_cleanup(handle);
			continue;
		}

		/* Get error code from easy handle and augment it if
		 * completing the download encounters further problems. */
		curl_ret = swupd_download_file_close(msg->data.result, &file->file);
		file->status = process_curl_error_codes(curl_ret, handle);
		debug("Curl - Complete ASYNC download: %s -> %s, status=%d\n", file->url, file->file.path, file->status);
		if (file->status == DOWNLOAD_STATUS_COMPLETED) {
			/* Wrap the success callback and schedule execution
			 * Results from the callback will be stored in multi_curl_file's cb_retval
			 * which is later checked for errors. */
			file->callback = h->success_cb;
			tp_task_schedule(h->thpool, (void *)success_callback_wrapper, (void *)file);
		} else {
			//Check if user can handle errors
			bool error_handled = h->error_cb && h->error_cb(file->status, file->data);
			enum retry_strategy strategy = determine_strategy(file->status);

			if (error_handled || strategy == DONT_RETRY) {
				// Don't retry download if error was handled
				file->retries = globals.max_retries;
				file->cb_retval = true;
				h->failed = list_prepend_data(h->failed, file);
			} else {
				//Download resume isn't supported. Disabling it for next try
				if (file->status == DOWNLOAD_STATUS_RANGE_ERROR) {
					h->resume_failed = true;
				}
			}
		}

		/* NOTE: Intentionally no removal of file from hashmap.  All
		 * needed files need determined and queued in one complete
		 * preparation phase.  Once all needed files are all present,
		 * they can be staged.  Otherwise a complex datastructure and
		 * retries are needed to insure only one download of a file
		 * happens fully to success AND a HASH.tar is uncompressed to
		 * and HASH and staged to the _multiple_ filenames with that
		 * hash. */

		curl_multi_remove_handle(h->mcurl, handle);
		curl_easy_cleanup(handle);
		file->curl = NULL;
		count++;

		/* A response has been completely processed by this point, so
		 * make the stack shorter. */
		h->mcurl_size--;
	}

	return 0;
}

static CURLMcode process_curl_multi_wait(CURLM *mcurl, bool *repeats)
{
	CURLMcode ret;
	int numfds = 0;

	ret = curl_multi_wait(mcurl, NULL, 0, CURL_MULTI_TIMEOUT, &numfds);

	// If numfds is zero curl_multi_wait reached a timeout or there isn't
	// any fd to process. The first ocurrence we assume it's a timeout.
	// After that we assume we need to force a sleep before calling
	// curl_multi_wait() again.
	if (ret != CURLM_OK && !numfds) {
		if (*repeats) {
			usleep(CURL_NOFD_WAIT);
		}
		*repeats = true;
	} else {
		*repeats = false;
	}

	return ret;
}

/*
 * Poll for downloads if the number of enqueued files are between the lower and
 * higher limits so we can have hysteresis in behavior.
 *
 * If the number of scheduled downloads is lower than xfer_queue_high the
 * function returns without processing the downloads.
 *
 * If the number of scheduled downloads is equal or higher than
 * xfer_queue_high we will process downloads and wait until the length of the
 * download queue is lower than xfer_queue_low.
 *
 * There's a protection to avoid having a download queue larger than
 * xfer_queue_high + XFER_QUEUE_BUFFER in cases where curl doesn't have any
 * data to process for any download. In this case we will return an error and
 * the caller is in charge of setting the last download to be retried.
 */
static int poll_fewer_than(struct swupd_curl_parallel_handle *h, size_t xfer_queue_high, size_t xfer_queue_low)
{
	CURLMcode curlm_ret;
	int running = 1;
	bool repeats = false;

	// Process pending curl writes and reads
	curlm_ret = curl_multi_perform(h->mcurl, &running);
	if (curlm_ret != CURLM_OK) {
		return -1;
	}

	// There are not enough transfers in queue yet
	if (h->mcurl_size < xfer_queue_high) {
		return 0;
	}

	/* When the high bound for the hysteresis is reached, process events
	 * until the low bound is reached */
	while (h->mcurl_size > xfer_queue_low) {

		// Wait for activity on the multi-stack
		curlm_ret = process_curl_multi_wait(h->mcurl, &repeats);
		if (curlm_ret != CURLM_OK) {
			return -1;
		}

		// Do more work before processing the queue
		curlm_ret = curl_multi_perform(h->mcurl, &running);
		if (curlm_ret != CURLM_OK) {
			return -1;
		}

		// Instead of using "numfds" as a hint for how many transfers
		// to process, try to drain the queue to the lower bound.
		int remaining = h->mcurl_size - xfer_queue_low;

		if (perform_curl_io_and_complete(h, remaining) != 0) {
			return -1;
		}

		if (!running) {
			break;
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
	// activity is reported the next XFER_QUEUE_BUFFER times. When an error is
	// returned, the most recent attempted transfer is added to the failed
	// list, later to be retried.
	if (xfer_queue_low != xfer_queue_high && h->mcurl_size > xfer_queue_high + XFER_QUEUE_BUFFER) {
		return -1;
	}

	return 0;
}

static int process_download(struct swupd_curl_parallel_handle *h, struct multi_curl_file *file)
{
	CURL *curl = NULL;
	CURLMcode curlm_ret = CURLM_OK;
	CURLcode curl_ret = CURLE_OK;
	struct stat stat;

	curl = curl_easy_init();
	if (curl == NULL) {
		goto out_bad;
	}
	file->curl = curl;

	if (h->progress_cb && file->progress) {

		curl_ret = curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, h->progress_cb);
		if (curl_ret != CURLE_OK) {
			goto out_bad;
		}

		/* switch on the progress meter */
		curl_ret = curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		if (curl_ret != CURLE_OK) {
			goto out_bad;
		}

		curl_ret = curl_easy_setopt(curl, CURLOPT_XFERINFODATA, file->progress);
		if (curl_ret != CURLE_OK) {
			goto out_bad;
		}
	}

	if (file->retries > 0 && !h->resume_failed && lstat(file->file.path, &stat) == 0 && file->status != DOWNLOAD_STATUS_RANGE_NOT_SATISFIABLE) {
		info("Curl - Resuming download for '%s'\n", file->url);
		curl_ret = curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)stat.st_size);
		if (curl_ret != CURLE_OK) {
			goto out_bad;
		}
		curl_ret = swupd_download_file_append(&file->file);
	} else {
		curl_ret = swupd_download_file_create(&file->file);
	}
	if (curl_ret != CURLE_OK) {
		goto out_bad;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_PRIVATE, (void *)file);
	if (curl_ret != CURLE_OK) {
		goto out_bad;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)file->file.fh);
	if (curl_ret != CURLE_OK) {
		goto out_bad;
	}

	curl_ret = swupd_curl_set_basic_options(curl, file->url, true);
	if (curl_ret != CURLE_OK) {
		goto out_bad;
	}

	debug("Curl - Start ASYNC download: %s -> %s\n", file->url, file->file.path);
	curlm_ret = curl_multi_add_handle(h->mcurl, curl);
	if (curlm_ret != CURLM_OK) {
		goto out_bad;
	}

	/* The current multi-stack size is not exposed by libcurl, so we track
	 * it with a counter variable. */
	h->mcurl_size++;

	return poll_fewer_than(h, h->max_xfer, h->max_xfer / 2);

out_bad:
	return -1;
}

static void swupd_curl_parallel_download_clear(struct swupd_curl_parallel_handle *h, int *num_downloads)
{
	struct multi_curl_file *file;
	int i, downloads = 0;
	struct list *l;

	curl_multi_cleanup(h->mcurl);
	tp_complete(h->thpool);
	list_free_list(h->failed);

	HASHMAP_FOREACH(h->curl_hashmap, i, l, file)
	{
		downloads++;
		free_curl_file(h, file);
	}
	hashmap_free(h->curl_hashmap);

	free(h);

	if (num_downloads) {
		*num_downloads = downloads;
	}
}

int swupd_curl_parallel_download_enqueue(struct swupd_curl_parallel_handle *h, const char *url, const char *filename, const char *hash, void *data)
{
	struct multi_curl_file *file;
	struct file_progress *progress;

	if (!h) {
		error("Curl - Invalid parallel download handle\n");
		return -EINVAL;
	}

	file = calloc(1, sizeof(struct multi_curl_file));
	ON_NULL_ABORT(file);

	file->file.path = strdup_or_die(filename);
	file->url = strdup_or_die(url);
	file->data = data;
	if (hash) {
		file->hash = hash;
		file->hash_key = HASH_TO_KEY(hash);
	} else {
		file->hash_key = hashmap_hash_from_string(filename);
	}

	if (!hashmap_put(h->curl_hashmap, file)) {
		// hash already in download queue
		free_curl_file(h, file);
		return 0;
	}

	unlink(file->file.path);

	progress = calloc(1, sizeof(struct file_progress));
	ON_NULL_ABORT(progress);
	progress->overall_progress = h->data;
	file->progress = progress;

	return process_download(h, file);
}

int swupd_curl_parallel_download_end(struct swupd_curl_parallel_handle *h, int *num_downloads)
{
	struct multi_curl_file *file;
	int i;
	struct list *l;
	bool retry = true;
	int ret = 0;

	if (!h) {
		error("Curl - Invalid parallel download handle\n");
		return -EINVAL;
	}

	while (poll_fewer_than(h, 0, 0) == 0 && retry) {
		retry = false;

		/* Wait for all threads to complete */
		if (tp_get_num_threads(h->thpool) > 0) {
			tp_complete(h->thpool);
			h->thpool = tp_start(1);
		}

		/* Check return values from threads, add failed items to h->failed list to retry */
		HASHMAP_FOREACH(h->curl_hashmap, i, l, file)
		{
			if (!file->cb_retval) {
				h->failed = list_prepend_data(h->failed, file);
			}
		}

		// The multi-stack is now emptied.
		perform_curl_io_and_complete(h, h->mcurl_size);

		//Retry failed downloads
		for (l = h->failed; l;) {
			struct multi_curl_file *file = l->data;

			if (file->retries < globals.max_retries &&
			    file->status != DOWNLOAD_STATUS_WRITE_ERROR) {
				struct list *next;

				file->retries++;

				//Remove item
				if (l == h->failed) {
					h->failed = l->next;
				}
				next = l->next;
				list_free_item(l, NULL);
				l = next;

				// Retry was probably scheduled because of network problems, so
				// reevaluate the number of parallel downloads
				reevaluate_number_of_parallel_downloads(h, file->retries);
				info("Curl - Starting download retry #%d for %s\n", file->retries, file->url);
				process_download(h, file);
				retry = true;
				continue;
			}
			l = l->next;
		}
		if (retry) {
			sleep(h->retry_delay);
			h->retry_delay = (h->retry_delay * DELAY_MULTIPLIER) > MAX_DELAY ? MAX_DELAY : (h->retry_delay * DELAY_MULTIPLIER);
		}
	}

	// If there are download failures at this point return an error
	if (h->failed != NULL) {
		ret = -SWUPD_COULDNT_DOWNLOAD_FILE;
	}

	swupd_curl_parallel_download_clear(h, num_downloads);
	return ret;
}

void swupd_curl_parallel_download_cancel(struct swupd_curl_parallel_handle *h)
{
	if (!h) {
		return;
	}

	swupd_curl_parallel_download_clear(h, NULL);
}
