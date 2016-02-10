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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <curl/curl.h>

#include "config.h"
#include "swupd.h"

static CURL *curl = NULL;

static int curr_version = -1;
static int req_version = -1;

int swupd_curl_init(void)
{
	CURLcode curl_ret;

	curl_ret = curl_global_init(CURL_GLOBAL_ALL);
	if (curl_ret != CURLE_OK) {
		return -1;
	}

	curl = curl_easy_init();
	if (curl == NULL) {
		curl_global_cleanup();
		return -1;
	}

	return 0;
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

static size_t swupd_download_version_to_memory(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	char *tmp_version = (char *)userdata;
	size_t data_len = size * nmemb;

	if (data_len >= LINE_MAX) {
		return 0;
	}

	memcpy(tmp_version, ptr, data_len);
	tmp_version[data_len] = '\0';

	return data_len;
}

/* curl easy CURLOPT_WRITEFUNCTION callback */
size_t swupd_download_file(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	struct file *file = (struct file*)userdata;
	const char *outfile;
	int fd;
	FILE *f;
	size_t written;

	outfile = file->staging;

	fd = open(outfile, O_CREAT | O_RDWR , 00600);
	if (fd < 0) {
		printf("Error: Cannot open %s for write: %s\n",
		       outfile, strerror(errno));
		return -1;
	}

	f = fdopen(fd, "a");
	if (!f) {
		printf("Error: Cannot fdopen %s for write: %s\n",
		       outfile, strerror(errno));
		close(fd);
		return -1;
	}

	written = fwrite(ptr, size*nmemb, 1, f);

	fflush(f);
	fclose(f);

	if (written != 1) {
		return -1;
	}

	return size*nmemb;
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
			char *in_memory_version_string, bool pack)
{
	CURLcode curl_ret;
	long ret = 0;
	int err;
	struct file *local = NULL;

	if (!curl) {
		abort();
	}
	curl_easy_reset(curl);

	if (in_memory_version_string == NULL) {
		// normal file download
		struct stat stat;

		if (file) {
			local = file;
		} else {
			local = calloc(1, sizeof(struct file));
			if (!local) {
				abort();
			}
		}
		local->staging = filename;

		if (lstat(filename, &stat) == 0) {
			if (pack) {
				curl_ret = curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, (curl_off_t) stat.st_size);
			} else {
				unlink(filename);
			}
		}

		curl_ret = curl_easy_setopt(curl, CURLOPT_PRIVATE, (void*)local);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
		curl_ret = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, swupd_download_file);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
		curl_ret = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)local);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
	} else {
		// only download latest version number, storing in the provided pointer
		printf("Attempting to download version string to memory\n");

		curl_ret = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, swupd_download_version_to_memory);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
		curl_ret = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)in_memory_version_string);
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
	if (curl_ret == CURLE_OK) {
		/* curl command succeeded, download might've failed, let our caller handle */
		switch (ret) {
			case 200:
			case 206:
				err = 0;
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
				printf("Curl: Could not resolve proxy\n");
				err = -1;
				break;
			case CURLE_COULDNT_RESOLVE_HOST:
				printf("Curl: Could not resolve host - '%s'\n", url);
				break;
			case CURLE_COULDNT_CONNECT:
				err = -ENONET;
				break;
			case CURLE_PARTIAL_FILE:
				printf("Curl: File incompletely downloaded from '%s' to '%s'\n",
							url, swupd_download_file);
				break;
			case CURLE_RECV_ERROR:
				printf("Curl: Failure receiving data from server\n");
				err = -ENOLINK;
				break;
			case CURLE_OPERATION_TIMEDOUT:
				printf("Curl: Communicating with server timed out.\n");
				err = -ETIMEDOUT;
				break;
			case CURLE_SSL_CACERT_BADFILE:
				printf("Curl: Bad SSL Cert file, cannot ensure secure connection\n");
				err = -1;
				break;
			default :
				printf("Curl error: %d - see curl.h for details\n", curl_ret);
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

	// TODO: change this to to use tlsv1.2 when it is supported and enabled
	curl_ret = curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_0);
	if (curl_ret != CURLE_OK) {
		goto exit;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_SSL_CIPHER_LIST, "HIGH");
	if (curl_ret != CURLE_OK) {
		goto exit;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_PINNEDPUBLICKEY, "/usr/share/clear/update-ca/425b0f6b.key");
	if (curl_ret != CURLE_OK) {
		goto exit;
	}

	curl_ret = curl_easy_setopt(curl, CURLOPT_CAPATH , UPDATE_CA_CERTS_PATH);
	if (curl_ret != CURLE_OK) {
		goto exit;
	}

	// TODO: add the below when you know the paths:
	//curl_easy_setopt(curl, CURLOPT_CRLFILE, path-to-cert-revoc-list);
	//if (curl_ret != CURLE_OK) {
	//	goto exit;
	//}

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

	if (update_server_port > 0) {
		curl_ret = curl_easy_setopt(curl, CURLOPT_PORT, update_server_port);
		if (curl_ret != CURLE_OK) {
			goto exit;
		}
	}

	if (strncmp(url, content_server_urls[1], strlen(content_server_urls[1])) == 0) {
#warning SECURITY HOLE since we can't SSL pin arbitrary servers
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

#warning setup a means to validate IPv6 works end to end
	curl_ret = curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
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
