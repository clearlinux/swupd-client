#ifndef __SWUPD_CURL__
#define __SWUPD_CURL__

#include "swupd_progress.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Struct to keep files downloaded to memory and used by
 * swupd_curl_get_file_memory()
 */
struct curl_file_data {
	size_t capacity;
	size_t len;
	char *data;
};

/* Download status code */
enum download_status {
	DOWNLOAD_STATUS_COMPLETED = 0,
	DOWNLOAD_STATUS_PARTIAL_FILE,
	DOWNLOAD_STATUS_NOT_FOUND,
	DOWNLOAD_STATUS_FORBIDDEN,
	DOWNLOAD_STATUS_TIMEOUT,
	DOWNLOAD_STATUS_RANGE_ERROR,
	DOWNLOAD_STATUS_WRITE_ERROR,
	DOWNLOAD_STATUS_ERROR,
};

/*
 * Handle for swupd curl parallel functions.
 */
struct swupd_curl_parallel_handle;

/*
 * Callback to be called when a download is successful.
 */
typedef bool (*swupd_curl_success_cb)(void *data);

/*
 * Callback to be called when a download has failed. 'status' indicates the
 * error occurred.
 */
typedef bool (*swupd_curl_error_cb)(enum download_status status, void *data);

/*
 * Callback called when 'data' is no longer needed, so user's can free it
 */
typedef void (*swupd_curl_free_cb)(void *data);

/*
 * Callback called periodically by curl to report on download progress
 */
typedef int (*swupd_curl_progress_cb)(void *clientp, int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow);

/*
 * Init the swupd curl.
 * Must be called before any curl operation.
 */
int swupd_curl_init(void);

/*
 * Close curl and release all memory allocated by swupd_curl_init().
 */
void swupd_curl_deinit(void);

/*
 * Query the file content size of 'url' without downloading the full file.
 */
double swupd_curl_query_content_size(char *url);

/*
 * Download 'url' and save it in 'filename'.
 *
 * Returns 0 on success and a negative number on errors
 */
int swupd_curl_get_file(const char *url, char *filename);

/*
 * Download 'url' to memory, saving it on 'file_data'
 *
 * Returns 0 on success and a negative number on errors
 */
int swupd_curl_get_file_memory(const char *url, struct curl_file_data *file_data);

/*
 * Start a parallel download element.
 *
 * Parameters:
 *  - max_xfer: The maximum number of simultaneos downloads.
 *
 * Parallel download handler will retry max_retries times to download each file,
 * ading a timeout between each try.
 *
 * Note: This function is non-blocking.
 */
struct swupd_curl_parallel_handle *swupd_curl_parallel_download_start(size_t max_xfer);

/*
 * Set parallel downloads callbacks.
 *
 *  - success_cb(): Called for each successful download, where data is the data
 *                  informed on swupd_curl_parallel_download_enqueue().
 *                  success_cb should return true if the download file was
 *                  handled correctly. Return false to schedule a retry.
 *  - error_cb():   Called for each failed download, where data is the data
 *                  informed on swupd_curl_parallel_download_enqueue()
 *                  and response is the HTTP response code. error_cb should
 *                  return true if the error was handled by caller or false
 *                  to schedule a retry.
 * - free_cb():     Called when data is ready to be freed.
 */
void swupd_curl_parallel_download_set_callbacks(struct swupd_curl_parallel_handle *handle, swupd_curl_success_cb success_cb, swupd_curl_error_cb error_cb, swupd_curl_free_cb free_cb);

/*
 * Set parallel downloads progress callback
 *
 *  - progress_cb(): Called periodically by curl while downloading/uploading files.
 *                   The return value of the function is not really important, the
 *                   function prints the download progress, so it will always return 0.
 *  - data:          User data to be informed to progress_cb.
 */
void swupd_curl_parallel_download_set_progress_callbacks(struct swupd_curl_parallel_handle *handle, swupd_curl_progress_cb progress_cb, void *data);

/*
 * Enqueue a file to be downloaded. If the number of current downloads is higher
 * than max_xfer, this function will be blocked for downloads until the number of
 * current downloads reach max_xfer / 2.
 *
 * Parameters:
 *  - handle: Handle created with swupd_curl_parallel_download_start().
 *  - url: The url to be downloaded.
 *  - filename: Full path of the filename to save the download content.
 *  - hash: Optional hex string with hash to be used as unique identifier of this
 *    file. If NULL, filename will be used as the identifier. String MUST contain
 *    only characters in '0123456789abcdef'.
 *  - data: User data to be informed to success_cb().
 *
 * Note: This function MAY be blocked.
 */
int swupd_curl_parallel_download_enqueue(struct swupd_curl_parallel_handle *handle, const char *url, const char *filename, const char *hash, void *data);

/*
 * Finish all pending downloads and free memory allocated by parallel download
 * handler.
 *
 * Parameters:
 *  - handle: Handle created with swupd_curl_parallel_download_start().
 *  - num_downloads: Optional int pointer to be filled with the number of
 *    files enqueued for download using this handler. Include failed downloads.
 *
 * Note: This function MAY be blocked.
 */
int swupd_curl_parallel_download_end(struct swupd_curl_parallel_handle *handle, int *num_downloads);

#ifdef __cplusplus
}
#endif

#endif
