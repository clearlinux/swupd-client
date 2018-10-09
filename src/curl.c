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

#define _GNU_SOURCE
#include <curl/curl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "curl-internal.h"
#include "swupd.h"

#define SWUPD_CURL_LOW_SPEED_LIMIT 1
#define SWUPD_CURL_CONNECT_TIMEOUT 30
#define SWUPD_CURL_RCV_TIMEOUT 120

static CURL *curl = NULL;

/* alternative CA Path */
static char *capath = NULL;

/* Pretty print curl return status */
static void swupd_curl_strerror(CURLcode curl_ret)
{
	fprintf(stderr, "Curl error: (%d) %s\n", curl_ret, curl_easy_strerror(curl_ret));
}

/*
 * Cannot avoid a TOCTOU here with the current curl API.  Only using the curl
 * API, a curl_easy_setopt does not detect if the client SSL certificate is
 * present on the filesystem.  This only happens during curl_easy_perform.
 * The emphasis is rather on how using an SSL client certificate is an opt-in
 * function rather than an opt-out function.
 */
CURLcode swupd_curl_set_optional_client_cert(CURL *curl)
{
	CURLcode curl_ret = CURLE_OK;
	char *client_cert_path;

	client_cert_path = mk_full_filename(path_prefix, SSL_CLIENT_CERT);
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
	free_string(&client_cert_path);
	return curl_ret;
}

static int check_connection(const char *test_capath)
{
	CURLcode curl_ret;

	curl_ret = curl_easy_setopt(curl, CURLOPT_URL, version_url);
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
	curl_ret = swupd_curl_set_optional_client_cert(curl);
	if (curl_ret != CURLE_OK) {
		return -1;
	}

	curl_ret = curl_easy_perform(curl);

	switch (curl_ret) {
	case CURLE_OK:
		return 0;
	case CURLE_SSL_CACERT:
		fprintf(stderr, "Error: unable to verify server SSL certificate\n");
		return -EBADCERT;
	case CURLE_SSL_CERTPROBLEM:
		fprintf(stderr, "Curl: Problem with the local client SSL certificate\n");
		return -EBADCERT;
	default:
		swupd_curl_strerror(curl_ret);
		/* something bad, stop */
		return -1;
	}
}

int swupd_curl_init(void)
{
	CURLcode curl_ret;
	char *str;
	char *tok;
	char *ctx;
	int ret;
	struct stat st;

	curl_ret = curl_global_init(CURL_GLOBAL_ALL);
	if (curl_ret != CURLE_OK) {
		fprintf(stderr, "Error: failed to initialize CURL environment\n");
		return -1;
	}

	curl = curl_easy_init();
	if (curl == NULL) {
		fprintf(stderr, "Error: failed to initialize CURL session\n");
		curl_global_cleanup();
		return -1;
	}

	ret = check_connection(NULL);
	if (ret == 0) {
		return 0;
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

			ret = check_connection(tok);
			if (ret == 0) {
				capath = strdup_or_die(tok);
				break;
			}
		}
		free_string(&str);
	}

	return ret;
}

void swupd_curl_deinit(void)
{
	if (curl) {
		curl_easy_cleanup(curl);
	}
	curl = NULL;

	free_string(&capath);

	curl_global_cleanup();
}

static size_t filesize_from_header_cb(void UNUSED_PARAM *func, size_t size, size_t nmemb, void UNUSED_PARAM *data)
{
	/* Drop the header, we just want file size */
	return (size_t)(size * nmemb);
}

double swupd_curl_query_content_size(char *url)
{
	CURLcode curl_ret;
	double content_size;

	if (!curl) {
		return -1;
	}

	curl_easy_reset(curl);

	/* Set buffer for error string */
	curl_ret = curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
	if (curl_ret != CURLE_OK) {
		return -1;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, filesize_from_header_cb);
	if (curl_ret != CURLE_OK) {
		return -1;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
	if (curl_ret != CURLE_OK) {
		return -1;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_URL, url);
	if (curl_ret != CURLE_OK) {
		return -1;
	}

	if (capath) {
		curl_ret = curl_easy_setopt(curl, CURLOPT_CAPATH, capath);
		if (curl_ret != CURLE_OK) {
			return -1;
		}
	}

	curl_ret = swupd_curl_set_optional_client_cert(curl);
	if (curl_ret != CURLE_OK) {
		return -1;
	}

	curl_ret = curl_easy_perform(curl);
	if (curl_ret != CURLE_OK) {
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
		fprintf(stderr, "Cannot open file for write \\*outfile=\"%s\",strerror=\"%s\"*\\\n",
			file->path, strerror(errno));
		return CURLE_WRITE_ERROR;
	}
	return CURLE_OK;
}

static CURLcode swupd_download_file_append(struct curl_file *file)
{
	file->fh = fopen(file->path, "a");
	if (!file->fh) {
		fprintf(stderr, "Cannot open file for append \\*outfile=\"%s\",strerror=\"%s\"*\\\n",
			file->path, strerror(errno));
		return CURLE_WRITE_ERROR;
	}
	return CURLE_OK;
}

CURLcode swupd_download_file_complete(CURLcode curl_ret, struct curl_file *file)
{
	if (file->fh) {
		if (fclose(file->fh)) {
			fprintf(stderr, "Cannot close file after write \\*outfile=\"%s\",strerror=\"%s\"*\\\n",
				file->path, strerror(errno));
			if (curl_ret == CURLE_OK) {
				curl_ret = CURLE_WRITE_ERROR;
			}
		}
		file->fh = NULL;
	}
	return curl_ret;
}

/*
 * Download a single file SYNCHRONOUSLY
 *
 * - If in_memory_file != NULL the file will be stored in memory and not on disk.
 * - If resume_ok == true and resume is supported, the function will resume an
 *   interrupted download if necessary.
 * - If failure to download, partial download is not deleted.
 *
 * Returns: Zero on success or a standard < 0 status code on errors.
 *
 * NOTE: See full_download() for multi/asynchronous downloading of fullfiles.
 */
static int swupd_curl_get_file_full(const char *url, char *filename,
				    struct curl_file_data *in_memory_file, bool resume_ok)
{
	static bool resume_download_supported = true;

	CURLcode curl_ret;
	long ret = 0;
	int err = -1;
	struct curl_file local = { 0 };
	bool local_download = strncmp(url, "file://", 7) == 0;

	if (!curl) {
		return -1;
	}
restart_download:
	curl_easy_reset(curl);

	if (!in_memory_file) {
		// normal file download
		struct stat stat;

		local.path = filename;

		if (resume_ok && resume_download_supported && lstat(filename, &stat) == 0) {
			curl_ret = curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)stat.st_size);
			if (curl_ret != CURLE_OK) {
				goto exit;
			}

			curl_ret = swupd_download_file_append(&local);
		} else {
			curl_ret = swupd_download_file_create(&local);
		}

		curl_ret = curl_easy_setopt(curl, CURLOPT_PRIVATE, (void *)&local);
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

	curl_ret = swupd_curl_set_basic_options(curl, url);
	if (curl_ret != CURLE_OK) {
		goto exit;
	}

	curl_ret = curl_easy_perform(curl);
	if (curl_ret == CURLE_OK || curl_ret == CURLE_HTTP_RETURNED_ERROR) {
		curl_ret = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &ret);
	}

exit:
	if (!in_memory_file) {
		curl_ret = swupd_download_file_complete(curl_ret, &local);
	}
	if (curl_ret == CURLE_OK) {
		/* curl command succeeded, download might've failed, let our caller handle */
		switch (ret) {
		case 200:
		case 206:
			err = 0;
			break;
		case 0:
			/* When using the FILE:// protocol, 0 indicates success.
			 * Otherwise, it means the web server hasn't responded yet.
			 */
			if (local_download) {
				err = 0;
			} else {
				err = -ETIMEDOUT;
			}
			break;
		case 403:
			err = -EACCES;
			break;
		case 404:
			err = -ENET404;
			break;
		default:
			err = -1;
			break;
		}
	} else { /* download failed but let our caller do it */
		switch (curl_ret) {
		case CURLE_COULDNT_RESOLVE_PROXY:
			fprintf(stderr, "Curl: Could not resolve proxy\n");
			err = -1;
			break;
		case CURLE_COULDNT_RESOLVE_HOST:
			fprintf(stderr, "Curl: Could not resolve host - '%s'\n", url);
			err = -1;
			break;
		case CURLE_COULDNT_CONNECT:
			fprintf(stderr, "Curl: Could not connect to host or proxy\n");
			err = -ENONET;
			break;
		case CURLE_FILE_COULDNT_READ_FILE:
			err = -ENOENT;
			break;
		case CURLE_PARTIAL_FILE:
			fprintf(stderr, "Curl: File incompletely downloaded from '%s' to '%s'\n",
				url, filename);
			err = -1;
			break;
		case CURLE_RECV_ERROR:
			fprintf(stderr, "Curl: Failure receiving data from server\n");
			err = -ENOLINK;
			break;
		case CURLE_WRITE_ERROR:
			fprintf(stderr, "Curl: Error downloading to local file - %s\n", filename);
			err = -EIO;
			break;
		case CURLE_OPERATION_TIMEDOUT:
			fprintf(stderr, "Curl: Communicating with server timed out.\n");
			err = -ETIMEDOUT;
			break;
		case CURLE_SSL_CACERT_BADFILE:
			fprintf(stderr, "Curl: Bad SSL Cert file, cannot ensure secure connection\n");
			err = -1;
			break;
		case CURLE_SSL_CERTPROBLEM:
			fprintf(stderr, "Curl: Problem with the local client SSL certificate\n");
			ret = -EBADCERT;
			break;
		case CURLE_RANGE_ERROR:
			fprintf(stderr, "Range command not supported by server, download resume disabled.\n");

			//Reset variables
			ret = 0;
			err = -1;
			memset(&local, 0, sizeof(local));

			//Disable download resume
			resume_download_supported = false;
			goto restart_download;
		default:
			swupd_curl_strerror(curl_ret);
			err = -1;
			break;
		}
	}

	if (err && !resume_ok) {
		unlink(filename);
	}

	return err;
}

/*
 * Download a single file SYNCHRONOUSLY
 *
 * Returns: Zero on success or a standard < 0 status code on errors.
 */
int swupd_curl_get_file(const char *url, char *filename)
{
	return swupd_curl_get_file_full(url, filename, NULL, false);
}

/*
 * Download a single file SYNCHRONOUSLY to a memory struct
 *
 * Returns: Zero on success or a standard < 0 status code on errors.
 */
int swupd_curl_get_file_memory(const char *url, struct curl_file_data *file_data)
{
	return swupd_curl_get_file_full(url, NULL, file_data, false);
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

CURLcode swupd_curl_set_basic_options(CURL *curl, const char *url)
{
	static bool use_ssl = true;

	CURLcode curl_ret = CURLE_OK;

	curl_ret = curl_easy_setopt(curl, CURLOPT_URL, url);
	if (curl_ret != CURLE_OK) {
		goto exit;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
	if (curl_ret != CURLE_OK && curl_ret != CURLE_UNSUPPORTED_PROTOCOL) {
		goto exit;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_PIPEWAIT, 1);
	if (curl_ret != CURLE_OK && curl_ret != CURLE_UNKNOWN_OPTION) {
		goto exit;
	}

	if (update_server_port > 0) {
		curl_ret = curl_easy_setopt(curl, CURLOPT_PORT, update_server_port);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
	}

	if (strncmp(url, "https://", 8) == 0) {
		//TODO: Fix "SECURITY HOLE since we can't SSL pin arbitrary servers"
		curl_ret = swupd_curl_set_security_opts(curl);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
	} else {
		if (use_ssl) {
			use_ssl = false;
		}
	}

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

	/* Avoid downloading HTML files for error responses if the HTTP code is >= 400 */
	curl_ret = curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	if (curl_ret != CURLE_OK) {
		goto exit;
	}

exit:
	return curl_ret;
}
