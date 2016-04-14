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
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "swupd-build-variant.h"
#include "swupd.h"

/* This file provides a managed download facility for code that needs a set of
 * update tar files.  Such code starts with one call to "start_full_download()",
 * then makes a series of calls to "full_download()" once per desired file, and
 * then finishes with one call to "end_full_downlad()" which will block until
 * the previously queued downloads are completed and and untarred.
 */

static CURLM *mcurl = NULL;
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
 * file->hash[0] acts as index into the arrays
 */
struct swupd_curl_hashbucket {
	pthread_mutex_t mutex;
	struct list *list;
};
#define SWUPD_CURL_HASH_BUCKETS 256
static struct swupd_curl_hashbucket swupd_curl_hashmap[SWUPD_CURL_HASH_BUCKETS];

/* try to insert the file into the hashmap download queue
 * returns 1 if no download is needed
 * returns 0 if download is needed
 * returns -1 if error */
static int swupd_curl_hashmap_insert(struct file *file)
{
	struct list *iter;
	struct file *tmp;
	char *tar_dotfile;
	char *targetfile;
	struct stat stat;
	int hashmap_index = file->hash[0];
	struct swupd_curl_hashbucket *bucket = &swupd_curl_hashmap[hashmap_index];

	pthread_mutex_lock(&bucket->mutex);

	iter = bucket->list;
	while (iter) {
		tmp = iter->data;
		if (hash_compare(tmp->hash, file->hash)) {
			// hash already in download queue
			pthread_mutex_unlock(&bucket->mutex);
			return 1;
		}
		iter = iter->next;
	}

	// if valid target file is already here, no need to download
	string_or_die(&targetfile, "%s/staged/%s", state_dir, file->hash);

	if (lstat(targetfile, &stat) == 0) {
		if (verify_file(file, targetfile)) {
			free(targetfile);
			pthread_mutex_unlock(&bucket->mutex);
			return 1;
		}
	}
	free(targetfile);

	// hash not in queue and not present in staged

	// clean up in case any prior download failed in a partial state
	string_or_die(&tar_dotfile, "%s/download/.%s.tar", state_dir, file->hash);
	unlink(tar_dotfile);
	free(tar_dotfile);

	// queue the hash for download
	iter = bucket->list;
	if ((iter = list_prepend_data(iter, file)) == NULL) {
		pthread_mutex_unlock(&bucket->mutex);
		return -1;
	}
	bucket->list = iter;

	pthread_mutex_unlock(&bucket->mutex);
	return 0;
}

/* hysteresis thresholds */
static int MAX_XFER = 25;
static int MAX_XFER_BOTTOM = 15;

int start_full_download(bool pipelining)
{
	int i;

	failed = NULL;
	for (i = 0; i < SWUPD_CURL_HASH_BUCKETS; i++) {
		pthread_mutex_init(&swupd_curl_hashmap[i].mutex, NULL);
	}

	mcurl = curl_multi_init();
	if (mcurl == NULL) {
		return -1;
	}

	/*
	 * we want to not do HTTP pipelining once things have failed once.. in case some transpoxy in the middle
	 * is even more broken than average. This at least will allow the user to update, albeit slowly.
	 */
	if (pipelining) {
		curl_multi_setopt(mcurl, CURLMOPT_PIPELINING, 1);
	} else {
		/* survival: don't go too parallel in verify/fix loop */
		MAX_XFER = 1;
		MAX_XFER_BOTTOM = 1;
	}

	printf("Starting download of remaining update content. This may take a while...\n");

	return 0;
}

static void free_curl_list_data(void *data)
{
	struct file *file = (struct file *)data;
	CURL *curl = file->curl;
	(void) swupd_download_file_complete(CURLE_OK, file);
	if (curl != NULL) {
		curl_multi_remove_handle(mcurl, curl);
		curl_easy_cleanup(curl);
	}
}

void clean_curl_multi_queue(void)
{
	int i;
	struct swupd_curl_hashbucket *bucket;

	for (i = 0; i < SWUPD_CURL_HASH_BUCKETS; i++) {
		bucket = &swupd_curl_hashmap[i];
		pthread_mutex_lock(&bucket->mutex);
		list_free_list_and_data(bucket->list, free_curl_list_data);
		bucket->list = NULL;
		pthread_mutex_unlock(&bucket->mutex);
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
	free(tarcommand);

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
	char *tarcommand;

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
			free(tar_dotfile);
			free(tarfile);
			free(targetfile);
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
		free(tar_dotfile);
		goto exit;
	}
	free(tar_dotfile);

	err = check_tarfile_content(file, tarfile);
	if (err) {
		goto exit;
	}

	/* modern tar will automatically determine the compression type used */
	string_or_die(&tarcommand, TAR_COMMAND " -C %s/staged/ " TAR_PERM_ATTR_ARGS " -xf %s 2> /dev/null",
		      state_dir, tarfile);

	err = system(tarcommand);
	if (WIFEXITED(err)) {
		err = WEXITSTATUS(err);
	}
	free(tarcommand);
	if (err) {
		printf("ignoring tar extract failure for fullfile %s.tar (ret %d)\n",
		       file->hash, err);
		goto exit;
		/* FIXME: can we respond meaningfully to tar error codes?
		 * symlink untars may have perm/xattr complaints and non-zero
		 * tar return, but symlink (probably?) untarred ok.
		 *
		 * Also getting complaints on some new regular files?
		 *
		 * Either way we verify the hash later, so on error there,
		 * something could try to recover? */
	} else {
		/* Only unlink when tar succeeded, so we can examine the tar file
		 * in the failure case. */
		unlink(tarfile);
	}

	err = lstat(targetfile, &stat);
exit:
	free(tarfile);
	free(targetfile);
	if (err) {
		unlink_all_staged_content(file);
	}
	return err;
}

static int perform_curl_io_and_complete(int *left)
{
	CURLMsg *msg;
	long ret;
	CURLMcode curlm_ret;
	CURLcode curl_ret;

	curlm_ret = curl_multi_perform(mcurl, left);
	if (curlm_ret != CURLM_OK) {
		return -1;
	}

	while (true) {
		CURL *handle;
		struct file *file;

		msg = curl_multi_info_read(mcurl, left);
		if (!msg) {
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

		/* Get error code from easy handle and augment it if
		 * completing the download encounters further problems. */
		curl_ret = msg->data.result;
		curl_ret = swupd_download_file_complete(curl_ret, file);

		/* The easy handle may have an error set, even if the server returns
		 * HTTP 200, so retry the download for this case. */
		if (ret == 200 && curl_ret != CURLE_OK) {
			printf("Error for %s download: %s\n", file->hash,
			       curl_easy_strerror(msg->data.result));
			failed = list_prepend_data(failed, file);
		} else if (ret == 200) {
			/* When both web server and CURL report success, only then
			 * proceed to uncompress. */
			untar_full_download(file);
		} else if (ret == 0) {
			/* When using the FILE:// protocol, 0 indicates success.
			 * Otherwise, it means the web server hasn't responded yet.
			 */
			if (local_download) {
				untar_full_download(file);
			} else {
				printf("error: received http \"0\" response for %s download\n",
				       file->hash);
				failed = list_prepend_data(failed, file);
			}
		} else {
			char *url = NULL;

			printf("error: received %ld response for download\n", ret);
			failed = list_prepend_data(failed, file);
			curl_easy_getinfo(handle, CURLINFO_EFFECTIVE_URL, &url);

			unlink_all_staged_content(file);
		}
		if (file->staging) {
			free(file->staging);
			file->staging = NULL;
		}

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
	}

	curlm_ret = curl_multi_perform(mcurl, left);
	if (curlm_ret != CURLM_OK) {
		return -1;
	}

	return 0;
}

/* 2 limits, so that we can have hysteresis in behavior. We let the caller
 * add new transfer up until the queue reaches the high threshold. At this point
 * we don't return to the caller and instead process the queue until its len
 * gets below the low threshold */
static int poll_fewer_than(int xfer_queue_high, int xfer_queue_low)
{
	int left;
	CURLMcode curlm_ret;

	curlm_ret = curl_multi_perform(mcurl, &left);
	if (curlm_ret != CURLM_OK) {
		return -1;
	}

	if (left <= xfer_queue_high) {
		return 0;
	}

	while (left > xfer_queue_low) {
		usleep(500); /* TODO this really ought to be a select() statement */
		if (perform_curl_io_and_complete(&left) != 0) {
			return -1;
		}
	}

	return 0;
}

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

	ret = swupd_curl_hashmap_insert(file);
	if (ret > 0) { /* no download needed */
		/* File already exists - report success */
		ret = 0;
		goto out_good;
	} else if (ret < 0) { /* error */
		goto out_bad;
	} /* else (ret == 0)	   download needed */

	curl = curl_easy_init();
	if (curl == NULL) {
		goto out_bad;
	}
	file->curl = curl;

	ret = poll_fewer_than(MAX_XFER, MAX_XFER_BOTTOM);
	if (ret) {
		clean_curl_multi_queue();
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
	curl_ret = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, swupd_download_file);
	if (curl_ret != CURLE_OK) {
		goto out_bad;
	}
	curl_ret = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)file);
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

	if (poll_fewer_than(MAX_XFER + 10, MAX_XFER) != 0) {
		clean_curl_multi_queue();
	}
	ret = 0;
	goto out_good;

out_bad:
	failed = list_prepend_data(failed, file);
	if (curl != NULL) {
		/* Must remove handle out of multi queue first!*/
		curl_multi_remove_handle(mcurl, curl);
		curl_easy_cleanup(curl);
	}
	free(filename);
out_good:
	free(url);
}

struct list *end_full_download(void)
{
	int left;
	int err;

	printf("Finishing download of update content...\n");

	if (poll_fewer_than(0, 0) == 0) {
		err = perform_curl_io_and_complete(&left);
		if (err) {
			clean_curl_multi_queue();
		}
	}

	curl_multi_cleanup(mcurl);
	return failed;
}
