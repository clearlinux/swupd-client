#ifndef __INCLUDE_GUARD_CURL_INTERNAL_H
#define __INCLUDE_GUARD_CURL_INTERNAL_H

/**
 * @file
 * @brief Define internal curl functions to be used by curl.c and download.c.
 *
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

/* values for retry strategies */
enum retry_strategy {
	DONT_RETRY = 0,
	RETRY_NOW,
	RETRY_WITH_DELAY
};

/**
 * @brief Create a file in disk to start the download.
 */
extern CURLcode swupd_download_file_create(struct curl_file *file);

/**
 * @brief Open a file for appending and start the download.
 */
extern CURLcode swupd_download_file_append(struct curl_file *file);

/**
 * @brief Close file after the download is finished.
 */
extern CURLcode swupd_download_file_close(CURLcode curl_ret, struct curl_file *file);

/**
 * @brief Set swupd default basic options to curl handler.
 */
extern CURLcode swupd_curl_set_basic_options(CURL *curl, const char *url, bool fail_on_error);

/**
 * @brief Process download curl return code 'curl_ret' and if needed the response code to
 * fill 'err' with an error code and return the status of this download.
 */
enum download_status process_curl_error_codes(int curl_ret, CURL *curl_handle);

/**
 * @brief Determines what strategy to use based on the return code:
 *
 * - do not retry: if the fault indicates that the failure isn't transient or
 *                 is unlikely to be successful if repeated (i.e. disk full)
 * - retry immediately: if the specific fault reported is unusual or rare, it
 *                      might have been caused by unusual circumstances, the
 *                      same failure is unlikely to be repeated (e.g. corrupt
 *                      network package)
 * - retry after a delay: if the fault is caused by a transient fault (like
 *                        network connectivity)
 */
enum retry_strategy determine_strategy(int status);

/**
 * @brief Init the swupd curl.
 *
 * @param url The url to use to test server connectivity
 *
 * @note Must be called before any curl operation.
 */
bool swupd_curl_init(const char *url);

#ifdef __cplusplus
}
#endif

#endif
