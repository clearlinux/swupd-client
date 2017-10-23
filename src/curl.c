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
 * use, you need to manage your own curl mutli environment.
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
#include "swupd.h"

static CURL *curl = NULL;

static int curr_version = -1;
static int req_version = -1;
static bool resume_download_enabled = true;

/* these are used to handle alternative trust locations */
static char *capath;
static char **fallback_capaths;
static char fallback_capaths_no;

/* Pretty print curl return status */
static void swupd_curl_strerror(CURLcode curl_ret)
{
	fprintf(stderr, "Curl error: (%d) %s\n", curl_ret, curl_easy_strerror(curl_ret));
}

int swupd_curl_init(void)
{
	CURLcode curl_ret;

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

#ifdef FALLBACK_CAPATHS
	/* parse and initialize CA paths. */
	{
		char *str = strdup(FALLBACK_CAPATHS);
		char *tok;
		char *ctx;
		struct stat st;
		int i;
		int sz = 1;

		fallback_capaths = (char **)malloc(sz * sizeof(char *));
		fallback_capaths[0] = NULL;
		i = 1;

		tok = strtok_r(str, ":", &ctx);
		do {
			if (stat(tok, &st)) {
				continue;
			}
			if ((st.st_mode & S_IFMT) != S_IFDIR) {
				continue;
			}
			if (i == sz) {
				sz <<= 1;
				fallback_capaths = (char **)realloc(fallback_capaths, sz * sizeof(char *));
			}
			fallback_capaths[i] = strdup(tok);
			i++;

		} while ((tok = strtok_r(NULL, ":", &ctx)));
		fallback_capaths_no = i;
		free(str);
	}
#else
	fallback_capaths = (char **)malloc(sizeof(char *));
	fallback_capaths[0] = NULL;
	fallback_capaths_no = 1;
#endif

	return 0;
}

static void swupd_curl_test_resume(void);

int swupd_curl_check_network(void)
{
	CURLcode curl_ret;
	int ret = -1;
	int i;
	CURL *c;
	static int has_network = 0;

	if (has_network)
		return 0;

	if (!curl) {
		return -1;
	}

	c = curl_easy_duphandle(curl);
	if (!c) {
		return -1;
	}

	for (i = 0; i < fallback_capaths_no; i++) {
		curl_easy_reset(c);

		curl_ret = curl_easy_setopt(c, CURLOPT_URL, version_url);
		if (curl_ret != CURLE_OK) {
			goto cleanup;
		}

		curl_ret = curl_easy_setopt(c, CURLOPT_NOBODY, 1L);
		if (curl_ret != CURLE_OK) {
			goto cleanup;
		}

		if (i) { /* first element represents default CApath, means no explicit setting */
			curl_ret = curl_easy_setopt(c, CURLOPT_CAPATH, fallback_capaths[i]);
			if (curl_ret != CURLE_OK) {
				break;
			}
		}

		curl_ret = curl_easy_perform(c);

		switch (curl_ret) {
		case CURLE_OK:
			capath = fallback_capaths[i];
			ret = 0;
			has_network = 1;
			swupd_curl_test_resume();
			goto cleanup;
		case CURLE_SSL_CACERT:
			break;
		default:
			swupd_curl_strerror(curl_ret);
			/* something bad, stop */
			goto cleanup;
		}
	}

cleanup:
	curl_easy_cleanup(c);
	return ret;
}

void swupd_curl_cleanup(void)
{
	if (curl) {
		curl_easy_cleanup(curl);
	}
	curl = NULL;
	curl_global_cleanup();
}

void swupd_curl_set_current_version(int v)
{
	curr_version = v;
}

void swupd_curl_set_requested_version(int v)
{
	req_version = v;
}

static size_t filesize_from_header_cb(void UNUSED_PARAM *func, size_t size, size_t nmemb, void UNUSED_PARAM *data)
{
	/* Drop the header, we just want file size */
	return (size_t)(size * nmemb);
}

double swupd_query_url_content_size(char *url)
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

static size_t swupd_download_version_to_memory(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	struct version_container *tmp_version = (struct version_container *)userdata;
	size_t data_len = size * nmemb;

	if (data_len + tmp_version->offset >= LINE_MAX) {
		return 0;
	}

	memcpy(tmp_version->version + tmp_version->offset, ptr, data_len);
	tmp_version->version[data_len + tmp_version->offset] = '\0';
	tmp_version->offset += data_len;

	return data_len;
}

CURLcode swupd_download_file_start(struct file *file)
{
	file->fh = fopen(file->staging, "w");
	if (!file->fh) {
		fprintf(stderr, "Cannot open file for write \\*outfile=\"%s\",strerror=\"%s\"*\\\n",
			file->staging, strerror(errno));
		return CURLE_WRITE_ERROR;
	}
	return CURLE_OK;
}

CURLcode swupd_download_file_complete(CURLcode curl_ret, struct file *file)
{
	if (file->fh) {
		if (fclose(file->fh)) {
			fprintf(stderr, "Cannot close file after write \\*outfile=\"%s\",strerror=\"%s\"*\\\n",
				file->staging, strerror(errno));
			if (curl_ret == CURLE_OK) {
				curl_ret = CURLE_WRITE_ERROR;
			}
		}
		file->fh = NULL;
	}
	return curl_ret;
}

/* Download a single file SYNCHRONOUSLY
 * - If (in_memory_version_string != NULL) the file downloaded is expected
 *   to be a version file and it is downloaded to memory instead of disk.
 * - Packs are big.  If (pack == true) then the function allows resuming
 *   a previous/interrupted download.
 * - This function returns zero or a standard < 0 status code.
 * - If failure to download, partial download is not deleted.
 * - NOTE: See full_download() for multi/asynchronous downloading of fullfiles.
 */
int swupd_curl_get_file(const char *url, char *filename, struct file *file,
			struct version_container *in_memory_version_string, bool pack)
{
	CURLcode curl_ret;
	long ret = 0;
	int err = -1;
	struct file *local = NULL;
	bool local_download = strncmp(url, "file://", 7) == 0;

	if (!curl) {
		abort();
	}
	curl_easy_reset(curl);

	if (in_memory_version_string == NULL) {
		// normal file download
		struct stat stat;

		if (file) {
			local = file;
			local->fh = NULL;
		} else {
			local = calloc(1, sizeof(struct file));
			if (!local) {
				abort();
			}
		}
		local->staging = filename;

		if (lstat(filename, &stat) == 0) {
			if (pack && resume_download_enabled) {
				curl_ret = curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)stat.st_size);
			} else {
				unlink(filename);
			}
		}

		curl_ret = curl_easy_setopt(curl, CURLOPT_PRIVATE, (void *)local);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
		curl_ret = swupd_download_file_start(local);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
		curl_ret = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)local->fh);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
	} else {
		// only download latest version number, storing in the provided pointer
		fprintf(stderr, "Attempting to download version string to memory\n");

		curl_ret = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, swupd_download_version_to_memory);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
		curl_ret = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)in_memory_version_string);
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
	if (local) {
		curl_ret = swupd_download_file_complete(curl_ret, local);
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
			err = -1;
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
		default:
			swupd_curl_strerror(curl_ret);
			err = -1;
			break;
		}
	}

	if (err) {
		if (!pack) {
			unlink(filename);
		}
	}

	if (local != file) {
		free(local);
	}

	return err;
}

static void swupd_curl_test_resume(void)
{
#define RESUME_BYTE_RANGE 2

	CURLcode curl_ret;
	char *url = NULL;

	if (!curl) {
		abort();
	}

	curl_easy_reset(curl);

	struct version_container tmp_version = { 0 };
	tmp_version.version = calloc(LINE_MAX, 1);
	if (tmp_version.version == NULL) {
		abort();
	}

	string_or_die(&url, "%s/version/format%s/latest", version_url, format_string);

	curl_ret = curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)RESUME_BYTE_RANGE);
	if (curl_ret != CURLE_OK) {
		goto exit;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, swupd_download_version_to_memory);
	if (curl_ret != CURLE_OK) {
		goto exit;
	}
	curl_ret = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&tmp_version);
	if (curl_ret != CURLE_OK) {
		goto exit;
	}
	curl_ret = curl_easy_setopt(curl, CURLOPT_COOKIE, "request=uncached");
	if (curl_ret != CURLE_OK) {
		goto exit;
	}

	curl_ret = swupd_curl_set_basic_options(curl, url);
	if (curl_ret != CURLE_OK) {
		goto exit;
	}

	curl_ret = curl_easy_perform(curl);

	if (curl_ret == CURLE_RANGE_ERROR) {
		fprintf(stderr, "Range command not supported by server, download resume disabled.\n");
		resume_download_enabled = false;
	}
exit:
	free(tmp_version.version);
	free(url);

	return;
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

#if 0
	// TODO: add the below when you know the paths:
	curl_easy_setopt(curl, CURLOPT_CRLFILE, path-to-cert-revoc-list);
	if (curl_ret != CURLE_OK) {
		goto exit;
	}
#endif

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
#warning "SECURITY HOLE since we can't SSL pin arbitrary servers"
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
