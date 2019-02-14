#ifndef __INCLUDE_GUARD_CURL_INTERNAL_H
#define __INCLUDE_GUARD_CURL_INTERNAL_H

/* Define internal curl functions to be used by curl.c and download.c.
 * Not expected to be used by users of swupd_curl_* functions.
 */

#include <curl/curl.h>

#include "swupd_curl.h"

#ifdef __cplusplus
extern "C" {
#endif

struct curl_file {
	char *path; /* output name used during download */
	FILE *fh;   /* file written into during downloading */
};

/*
 * Create a file in disk to start the download.
 */
extern CURLcode swupd_download_file_create(struct curl_file *file);

/*
 * Open a file for appending and start the download.
 */
extern CURLcode swupd_download_file_append(struct curl_file *file);

/*
 * Close file after the download is finished.
 */
extern CURLcode swupd_download_file_close(CURLcode curl_ret, struct curl_file *file);

/*
 * Set swupd default basic options to curl handler.
 */
extern CURLcode swupd_curl_set_basic_options(CURL *curl, const char *url, bool fail_on_error);

/* Process download curl return code 'curl_ret' and if needed the response code to
 * fill 'err' with an error code and return the status of this download. */
enum download_status process_curl_error_codes(int curl_ret, CURL *curl_handle);

#ifdef __cplusplus
}
#endif

#endif
