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

/*
 * The curl library is great, but it is a little bit of a pain to get it to
 * reuse connections properly for simple cases. This file will manage our
 * curl handle properly so that we have a standing chance to get reuse
 * of our connections.
 *
 * NOTE NOTE NOTE
 *
 * Only use these from the main thread of the program. For multithreaded
 * use, you need to manage your own curl multi environment.
 */

#include <curl/curl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "swupd.h"
#include "swupd_curl_internal.h"

#define SWUPD_CURL_LOW_SPEED_LIMIT 1
#define SWUPD_CURL_CONNECT_TIMEOUT 30
#define SWUPD_CURL_RCV_TIMEOUT 120

static CURL *curl = NULL;

uint64_t total_curl_sz = 0;

/* alternative CA Path */
static char *capath = NULL;

/* Progress callback and data */
static swupd_curl_progress_cb progress_cb = NULL;
static void *progress_cb_data = NULL;

/* Pretty print curl return status */
static void swupd_curl_strerror(CURLcode curl_ret)
{
	error("Curl - Download error - (%d) %s\n", curl_ret, curl_easy_strerror(curl_ret));
}

void swupd_curl_download_set_progress_callback(swupd_curl_progress_cb user_progress_cb, void *data)
{
	progress_cb = user_progress_cb;
	progress_cb_data = data;
}

/*
 * Cannot avoid a TOCTOU here with the current curl API.  Only using the curl
 * API, a curl_easy_setopt does not detect if the client SSL certificate is
 * present on the filesystem.  This only happens during curl_easy_perform.
 * The emphasis is rather on how using an SSL client certificate is an opt-in
 * function rather than an opt-out function.
 */
static CURLcode swupd_curl_set_optional_client_cert(CURL *curl)
{
	CURLcode curl_ret = CURLE_OK;
	char *client_cert_path;

	client_cert_path = sys_path_join("%s/%s", globals.path_prefix, SSL_CLIENT_CERT);
	if (access(client_cert_path, F_OK) == 0) {
		curl_ret = curl_easy_setopt(curl, CURLOPT_SSLCERT, client_cert_path);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
		curl_ret = curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
	}

exit:
	free_and_clear_pointer(&client_cert_path);
	return curl_ret;
}

static CURLcode swupd_curl_set_timeouts(CURL *curl)
{
	CURLcode curl_ret;

	curl_ret = curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, SWUPD_CURL_CONNECT_TIMEOUT);
	if (curl_ret != CURLE_OK) {
		goto exit;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, SWUPD_CURL_LOW_SPEED_LIMIT);
	if (curl_ret != CURLE_OK) {
		goto exit;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, SWUPD_CURL_RCV_TIMEOUT);
	if (curl_ret != CURLE_OK) {
		goto exit;
	}

exit:
	return curl_ret;
}

static int check_connection_capath(const char *test_capath, const char *url)
{
	CURLcode curl_ret;
	long response = 0;

	if (!url) {
		debug("Please provide the url to use to test the connection\n");
		return -1;
	}

	curl_easy_reset(curl);

	debug("Curl - check_connection url: %s\n", url);
	curl_ret = swupd_curl_set_basic_options(curl, url, false);
	if (curl_ret != CURLE_OK) {
		return -1;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
	if (curl_ret != CURLE_OK) {
		return -1;
	}

	if (test_capath) {
		curl_ret = curl_easy_setopt(curl, CURLOPT_CAPATH, test_capath);
		if (curl_ret != CURLE_OK) {
			return -1;
		}
	}

	curl_ret = curl_easy_perform(curl);

	switch (curl_ret) {
	case CURLE_OK:
		/* Note: when using http and the user is behind a proxy, we may get
		 * a valid response here from the proxy even if the http server is
		 * down, allow it to continue for now, it will fail further in the process */
		return 0;
	case CURLE_SSL_CACERT:
		debug("Curl - Unable to verify server SSL certificate\n");
		return -SWUPD_BAD_CERT;
	case CURLE_SSL_CERTPROBLEM:
		debug("Curl - Problem with the local client SSL certificate\n");
		return -SWUPD_BAD_CERT;
	case CURLE_OPERATION_TIMEDOUT:
		debug("Curl - Timed out\n");
		return -CURLE_OPERATION_TIMEDOUT;
	case CURLE_HTTP_RETURNED_ERROR:
		if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response) != CURLE_OK) {
			response = 0;
		}
		debug("Curl - process_curl_error_codes: curl_ret = %d, response = %ld\n", curl_ret, response);
		return -1;
	default:
		debug("Curl - Download error - (%d) %s\n", curl_ret,
		      curl_easy_strerror(curl_ret));
		return -1;
	}
}

int check_connection(char *url)
{
	if (!curl) {
		return swupd_curl_init(url) ? 0 : -1;
	}

	return check_connection_capath(NULL, url);
}

static bool perform_curl_init(const char *url)
{
	CURLcode curl_ret;
	char *str;
	char *tok;
	char *ctx = NULL;
	int ret;
	struct stat st;
	debug("Initializing swupd curl library\n");

	curl_ret = curl_global_init(CURL_GLOBAL_ALL);
	if (curl_ret != CURLE_OK) {
		error("Curl - Failed to initialize environment\n");
		return false;
	}

	curl = curl_easy_init();
	if (curl == NULL) {
		error("Curl - Failed to initialize session\n");
		curl_global_cleanup();
		return false;
	}

	ret = check_connection_capath(NULL, url);
	if (ret == 0) {
		return true;
	} else if (ret == -CURLE_OPERATION_TIMEDOUT) {
		error("Curl - Communicating with server timed out\n");
		ret = -1;
		goto exit;
	}

	if (FALLBACK_CAPATHS[0]) {
		str = strdup_or_die(FALLBACK_CAPATHS);
		for (tok = strtok_r(str, ":", &ctx); tok; tok = strtok_r(NULL, ":", &ctx)) {
			if (stat(tok, &st)) {
				continue;
			}
			if ((st.st_mode & S_IFMT) != S_IFDIR) {
				continue;
			}

			debug("Curl - Trying fallback CA path %s\n", tok);
			ret = check_connection_capath(tok, url);
			if (ret == 0) {
				capath = strdup_or_die(tok);
				break;
			}
		}
		free_and_clear_pointer(&str);
	}

exit:
	if (ret != 0) {
		/* curl failed to initialize */
		error("Failed to connect to update server: %s\n", url);
		info("Possible solutions for this problem are:\n"
		     "\tCheck if your network connection is working\n"
		     "\tFix the system clock\n"
		     "\tRun 'swupd info' to check if the urls are correct\n"
		     "\tCheck if the server SSL certificate is trusted by your system ('clrtrust generate' may help)\n");
		swupd_curl_cleanup();
		return false;
	}

	return true;
}

bool swupd_curl_init(const char *url)
{
	static bool initialized = false;

	if (initialized) {
		return curl != NULL;
	}

	initialized = true;
	return perform_curl_init(url);
}

void swupd_curl_cleanup(void)
{
	if (!curl) {
		return;
	}

	curl_easy_cleanup(curl);
	curl = NULL;
	free_and_clear_pointer(&capath);
	curl_global_cleanup();
}

static size_t dummy_write_cb(void UNUSED_PARAM *func, size_t size, size_t nmemb, void UNUSED_PARAM *data)
{
	/* Drop the content */
	return (size_t)(size * nmemb);
}

double swupd_curl_query_content_size(char *url)
{
	CURLcode curl_ret;
	double content_size;
	long response = 0;

	if (!swupd_curl_init(url)) {
		return -1;
	}

	curl_easy_reset(curl);

	curl_ret = swupd_curl_set_basic_options(curl, url, true);
	if (curl_ret != CURLE_OK) {
		return -1;
	}

	/* Set buffer for error string */
	curl_ret = curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
	if (curl_ret != CURLE_OK) {
		return -1;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, dummy_write_cb);
	if (curl_ret != CURLE_OK) {
		return -1;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
	if (curl_ret != CURLE_OK) {
		return -1;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dummy_write_cb);
	if (curl_ret != CURLE_OK) {
		return -1;
	}

	curl_ret = curl_easy_perform(curl);
	if (curl_ret != CURLE_OK) {
		return -1;
	}

	/* if the file doesn't exist in the remote server, the server will respond
	 * with its "404 Not Found" page, which has a size itself (162 bytes for
	 * NGINX which is the default content server used by clear). So if the file is
	 * not found we need to return a size of 0 instead, otherwise the download size
	 * calculation will be wrong */
	curl_ret = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response);
	if (curl_ret != CURLE_OK) {
		return -1;
	}
	/* only acceptable responses at this point are 0, 200 and 404 */
	if (response == 404) {
		/* file not found, size zero */
		return 0;
	} else if (response != 200 && response != 0) {
		/* some other error happened with the request, this may cause the
		 * download size to be wrong, so just return an error */
		return -1;
	}

	curl_ret = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_size);
	if (curl_ret != CURLE_OK) {
		return -1;
	}

	return content_size;
}

static size_t swupd_download_file_to_memory(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	struct curl_file_data *file_data = (struct curl_file_data *)userdata;
	size_t data_len = size * nmemb;

	if (data_len + file_data->len > file_data->capacity) {
		return 0;
	}

	memcpy(file_data->data + file_data->len, ptr, data_len);
	file_data->len += data_len;

	return data_len;
}

CURLcode swupd_download_file_create(struct curl_file *file)
{
	file->fh = fopen(file->path, "w");
	if (!file->fh) {
		error("Curl - Cannot open file for write \\*outfile=\"%s\",strerror=\"%s\"*\\\n",
		      file->path, strerror(errno));
		return CURLE_WRITE_ERROR;
	}
	return CURLE_OK;
}

CURLcode swupd_download_file_append(struct curl_file *file)
{
	file->fh = fopen(file->path, "a");
	if (!file->fh) {
		error("Curl - Cannot open file for append \\*outfile=\"%s\",strerror=\"%s\"*\\\n",
		      file->path, strerror(errno));
		return CURLE_WRITE_ERROR;
	}
	return CURLE_OK;
}

CURLcode swupd_download_file_close(CURLcode curl_ret, struct curl_file *file)
{
	if (file->fh) {
		if (fclose(file->fh)) {
			error("Curl - Cannot close file after write \\*outfile=\"%s\",strerror=\"%s\"*\\\n",
			      file->path, strerror(errno));
			if (curl_ret == CURLE_OK) {
				curl_ret = CURLE_WRITE_ERROR;
			}
		}
		file->fh = NULL;
	}
	return curl_ret;
}

enum download_status process_curl_error_codes(int curl_ret, CURL *curl_handle)
{
	char *url;
	if (curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &url) != CURLE_OK) {
		url = "<not available>";
	}

	/*
	 * retrieve bytes transferred, errors or not
	 */
	curl_off_t curl_sz = 0;
	if (curl_easy_getinfo(curl_handle, CURLINFO_SIZE_DOWNLOAD_T, &curl_sz) == CURLE_OK) {
		total_curl_sz += curl_sz;
	}

	if (curl_ret == CURLE_OK || curl_ret == CURLE_HTTP_RETURNED_ERROR) {
		long response = 0;
		if (curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response) != CURLE_OK) {
			response = -1; // Force error
		}
		debug("Curl - process_curl_error_codes: curl_ret = %d, response = %ld\n", curl_ret, response);
		/* curl command succeeded, download might've failed, let our caller handle */
		switch (response) {
		case 206:
			error("Curl - Partial file downloaded from '%s'\n", url);
			return DOWNLOAD_STATUS_PARTIAL_FILE;
		case 200:
		case 0:
			return DOWNLOAD_STATUS_COMPLETED;
		case 403:
			debug("Curl - Download failed - forbidden (403) - '%s'\n", url);
			return DOWNLOAD_STATUS_FORBIDDEN;
		case 404:
			debug("Curl - Download failed - file not found (404) - '%s'\n", url);
			return DOWNLOAD_STATUS_NOT_FOUND;
		case 416:
			debug("Curl - Download failed - range not satisfiable (416) - '%s'\n", url);
			return DOWNLOAD_STATUS_RANGE_NOT_SATISFIABLE;
		default:
			error("Curl - Download failed: response (%ld) -  '%s'\n", response, url);
			return DOWNLOAD_STATUS_ERROR;
		}
	} else { /* download failed but let our caller do it */
		debug("Curl - process_curl_error_codes - curl_ret = %d\n", curl_ret);
		switch (curl_ret) {
		case CURLE_COULDNT_RESOLVE_PROXY:
			error("Curl - Could not resolve proxy\n");
			return DOWNLOAD_STATUS_SERVER_UNREACHABLE;
		case CURLE_COULDNT_RESOLVE_HOST:
			error("Curl - Could not resolve host - '%s'\n", url);
			return DOWNLOAD_STATUS_SERVER_UNREACHABLE;
		case CURLE_COULDNT_CONNECT:
			error("Curl - Could not connect to host or proxy - '%s'\n", url);
			return DOWNLOAD_STATUS_SERVER_UNREACHABLE;
		case CURLE_FILE_COULDNT_READ_FILE:
			return DOWNLOAD_STATUS_NOT_FOUND;
		case CURLE_PARTIAL_FILE:
			error("Curl - File incompletely downloaded - '%s'\n", url);
			return DOWNLOAD_STATUS_ERROR;
		case CURLE_RECV_ERROR:
			error("Curl - Failure receiving data from server - '%s'\n", url);
			return DOWNLOAD_STATUS_ERROR;
		case CURLE_WRITE_ERROR:
			error("Curl - Error downloading to local file - '%s'\n", url);
			error("Curl - Check free space for %s?\n", globals.state_dir);
			return DOWNLOAD_STATUS_WRITE_ERROR;
		case CURLE_OPERATION_TIMEDOUT:
			error("Curl - Communicating with server timed out - '%s'\n", url);
			return DOWNLOAD_STATUS_TIMEOUT;
		case CURLE_SSL_CACERT_BADFILE:
			error("Curl - Bad SSL Cert file, cannot ensure secure connection - '%s'\n", url);
			return DOWNLOAD_STATUS_ERROR;
		case CURLE_SSL_CERTPROBLEM:
			error("Curl - Problem with the local client SSL certificate - '%s'\n", url);
			return DOWNLOAD_STATUS_ERROR;
		case CURLE_RANGE_ERROR:
			error("Curl - Range command not supported by server, download resume disabled - '%s'\n", url);
			return DOWNLOAD_STATUS_RANGE_ERROR;
		default:
			swupd_curl_strerror(curl_ret);
		}
	}
	return DOWNLOAD_STATUS_ERROR;
}

/*
 * Download a single file SYNCHRONOUSLY
 *
 * - If in_memory_file != NULL the file will be stored in memory and not on disk.
 * - If resume_ok == true and resume is supported, the function will resume an
 *   interrupted download if necessary.
 * - If failure to download, partial download is not deleted.
 *
 * Returns: Zero (DOWNLOAD_STATUS_COMPLETED) on success or a status code > 0 on errors.
 *
 * NOTE: See full_download() for multi/asynchronous downloading of fullfiles.
 */
static enum download_status swupd_curl_get_file_full(const char *url, char *filename,
						     struct curl_file_data *in_memory_file, bool resume_ok)
{
	static bool resume_download_supported = true;

	CURLcode curl_ret;
	enum download_status status;
	struct curl_file local = { 0 };
	void *local_ptr = &local;

restart_download:
	curl_easy_reset(curl);

	if (!in_memory_file) {
		// normal file download
		struct stat stat;

		local.path = filename;

		if (resume_ok && resume_download_supported && lstat(filename, &stat) == 0) {
			info("Curl - Resuming download for '%s'\n", url);
			curl_ret = curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)stat.st_size);
			if (curl_ret != CURLE_OK) {
				goto exit;
			}

			curl_ret = swupd_download_file_append(&local);
		} else {
			curl_ret = swupd_download_file_create(&local);
		}
		if (curl_ret != CURLE_OK) {
			goto exit;
		}

		curl_ret = curl_easy_setopt(curl, CURLOPT_PRIVATE, (void *)local_ptr);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
		curl_ret = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)local.fh);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
	} else {
		curl_ret = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, swupd_download_file_to_memory);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
		curl_ret = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)in_memory_file);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
		curl_ret = curl_easy_setopt(curl, CURLOPT_COOKIE, "request=uncached");
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
	}

	curl_ret = swupd_curl_set_basic_options(curl, url, true);
	if (curl_ret != CURLE_OK) {
		goto exit;
	}

	debug("Curl - Start sync download: %s -> %s\n", url, in_memory_file ? "<memory>" : filename);
	curl_ret = curl_easy_perform(curl);

exit:
	if (!in_memory_file) {
		curl_ret = swupd_download_file_close(curl_ret, &local);
	}

	status = process_curl_error_codes(curl_ret, curl);
	debug("Curl - Complete sync download: %s -> %s, status=%d\n", url, in_memory_file ? "<memory>" : filename, status);
	if (status == DOWNLOAD_STATUS_RANGE_ERROR) {
		// Reset variable
		memset(&local, 0, sizeof(local));

		//Disable download resume
		resume_download_supported = false;
		goto restart_download;
	}
	if (status != DOWNLOAD_STATUS_COMPLETED && !resume_ok) {
		unlink(filename);
	}

	return status;
}

enum retry_strategy determine_strategy(int status)
{
	/* we don't need to retry if the content URL is local */
	if (globals.content_url_is_local) {
		return DONT_RETRY;
	}

	switch (status) {
	case DOWNLOAD_STATUS_FORBIDDEN:
	case DOWNLOAD_STATUS_NOT_FOUND:
	case DOWNLOAD_STATUS_WRITE_ERROR:
	case DOWNLOAD_STATUS_SERVER_UNREACHABLE:
		return DONT_RETRY;
	case DOWNLOAD_STATUS_RANGE_ERROR:
	case DOWNLOAD_STATUS_PARTIAL_FILE:
		return RETRY_NOW;
	case DOWNLOAD_STATUS_ERROR:
	case DOWNLOAD_STATUS_TIMEOUT:
		return RETRY_WITH_DELAY;
	default:
		return RETRY_NOW;
	}
}

static int retry_download_loop(const char *url, char *filename, struct curl_file_data *in_memory_file, bool resume_ok)
{

	int current_retry = 0;
	int sleep_time = globals.retry_delay;
	int strategy;
	int ret;

	if (!swupd_curl_init(url)) {
		return -ECOMM;
	}

	for (;;) {

		/* download file */
		ret = swupd_curl_get_file_full(url, filename, in_memory_file, resume_ok);

		if (ret == DOWNLOAD_STATUS_COMPLETED) {
			/* operation successful */
			break;
		}

		/* operation failed, determine retry strategy */
		current_retry++;
		strategy = determine_strategy(ret);
		if (strategy == DONT_RETRY) {
			/* don't retry just return a failure */
			return -EIO;
		}

		/* if we got to this point and we haven't reached the retry limit,
		 * we need to retry, otherwise just return the failure */
		if (strategy == RETRY_NOW) {
			sleep_time = 0;
		}
		if (globals.max_retries) {
			if (current_retry <= globals.max_retries) {
				if (sleep_time) {
					info("Waiting %d seconds before retrying the download\n", sleep_time);
				}
				sleep(sleep_time);
				sleep_time = (sleep_time * DELAY_MULTIPLIER) > MAX_DELAY ? MAX_DELAY : (sleep_time * DELAY_MULTIPLIER);
				info("Retry #%d downloading from %s\n", current_retry, url);
				continue;
			} else {
				warn("Maximum number of retries reached\n");
			}
		} else {
			info("Download retries is disabled\n");
		}
		/* we ran out of retries, return an error */
		return -ECOMM;
	}

	return ret;
}

/*
 * Download a single file SYNCHRONOUSLY
 *
 * Returns: Zero on success or a standard < 0 status code on errors.
 */
int swupd_curl_get_file(const char *url, char *filename)
{
	int ret;

	ret = retry_download_loop(url, filename, NULL, false);
	clean_spinner();

	return ret;
}

/*
 * Download a single file SYNCHRONOUSLY to a memory struct
 *
 * Returns: Zero on success or a standard < 0 status code on errors.
 */
int swupd_curl_get_file_memory(const char *url, struct curl_file_data *file_data)
{
	int ret;

	ret = retry_download_loop(url, NULL, file_data, false);
	clean_spinner();

	return ret;
}

static CURLcode swupd_curl_set_security_opts(CURL *curl)
{
	CURLcode curl_ret = CURLE_OK;

	curl_ret = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, true);
	if (curl_ret != CURLE_OK) {
		goto exit;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
	if (curl_ret != CURLE_OK) {
		goto exit;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
	if (curl_ret != CURLE_OK) {
		goto exit;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_SSL_CIPHER_LIST, "HIGH");
	if (curl_ret != CURLE_OK) {
		goto exit;
	}

	if (capath) {
		curl_ret = curl_easy_setopt(curl, CURLOPT_CAPATH, capath);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
	}

	curl_ret = swupd_curl_set_optional_client_cert(curl);
	if (curl_ret != CURLE_OK) {
		goto exit;
	}

exit:
	return curl_ret;
}

CURLcode swupd_curl_set_basic_options(CURL *curl, const char *url, bool fail_on_error)
{
	CURLcode curl_ret = CURLE_OK;

	curl_ret = curl_easy_setopt(curl, CURLOPT_URL, url);
	if (curl_ret != CURLE_OK) {
		goto exit;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
	if (curl_ret != CURLE_OK && curl_ret != CURLE_UNSUPPORTED_PROTOCOL) {
		goto exit;
	}

	//TODO: Introduce code back on bug is fixed on curl
	//curl_ret = curl_easy_setopt(curl, CURLOPT_PIPEWAIT, 1);
	//if (curl_ret != CURLE_OK && curl_ret != CURLE_UNKNOWN_OPTION) {
	//	goto exit;
	//}

	curl_ret = curl_easy_setopt(curl, CURLOPT_USERAGENT, PACKAGE "/" VERSION);
	if (curl_ret != CURLE_OK && curl_ret != CURLE_UNKNOWN_OPTION) {
		debug("Curl - User agent not set");
		// Don't abort operation because this isn't a critical information
	}

	if (globals.update_server_port > 0) {
		curl_ret = curl_easy_setopt(curl, CURLOPT_PORT, globals.update_server_port);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
	}

	if (str_starts_with(url, "https://") == 0) {
		//TODO: Fix "SECURITY HOLE since we can't SSL pin arbitrary servers"
		curl_ret = swupd_curl_set_security_opts(curl);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
	}

	curl_ret = swupd_curl_set_timeouts(curl);
	if (curl_ret != CURLE_OK) {
		goto exit;
	}

	if (fail_on_error) {
		/* Avoid downloading HTML files for error responses if the HTTP code is >= 400 */
		curl_ret = curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
	}

	if (progress_cb) {
		curl_ret = curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}

		curl_ret = curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION,
					    progress_cb);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}

		curl_ret = curl_easy_setopt(curl, CURLOPT_XFERINFODATA, progress_cb_data);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
	}

exit:
	return curl_ret;
}

bool curl_is_url_local(const char *url)
{
	return str_starts_with(url, "file://") == 0;
}
