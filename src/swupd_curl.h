#ifndef __SWUPD_CURL__
#define __SWUPD_CURL__

/**
 * @file
 * @brief Wrapper to curl API calls.
 */

#include "swupd_progress.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Struct to keep files downloaded to memory and used by
 * swupd_curl_get_file_memory()
 */
struct curl_file_data {
	size_t capacity;
	size_t len;
	char *data;
};

/** @brief Download status code */
enum download_status {
	DOWNLOAD_STATUS_COMPLETED = 0,
	DOWNLOAD_STATUS_PARTIAL_FILE,
	DOWNLOAD_STATUS_NOT_FOUND,
	DOWNLOAD_STATUS_FORBIDDEN,
	DOWNLOAD_STATUS_TIMEOUT,
	DOWNLOAD_STATUS_RANGE_ERROR,
	DOWNLOAD_STATUS_WRITE_ERROR,
	DOWNLOAD_STATUS_ERROR,
	DOWNLOAD_STATUS_SERVER_UNREACHABLE,
};

/**
 * @brief Handle for swupd curl parallel functions.
 */
struct swupd_curl_parallel_handle;

/**
 * @brief Callback to be called when a download is successful.
 */
typedef bool (*swupd_curl_success_cb)(void *data);

/**
 * @brief Callback to be called when a download has failed. 'status' indicates the
 * error occurred.
 */
typedef bool (*swupd_curl_error_cb)(enum download_status status, void *data);

/**
 * @brief Callback called when 'data' is no longer needed, so user's can free it
 */
typedef void (*swupd_curl_free_cb)(void *data);

/**
 * @brief Callback called periodically by curl to report on download progress
 */
typedef int (*swupd_curl_progress_cb)(void *clientp, int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow);

/**
 * @brief Test a connection with a server.
 *
 * @param url The url to use to test server connectivity
 *
 * @returns 0 if the server is responding
 */
int check_connection(char *url);

/**
 * @brief Clean any curl initialization data, if needed.
 */
void swupd_curl_cleanup(void);

/**
 * @brief Query the file content size of 'url' without downloading the full file.
 */
double swupd_curl_query_content_size(char *url);

/**
 * @brief Download @c url and save it in @c filename.
 *
 * @returns 0 on success and a negative number on errors
 */
int swupd_curl_get_file(const char *url, char *filename);

/**
 * @brief Download @c url to memory, saving it on @c file_data
 *
 * @returns 0 on success and a negative number on errors
 */
int swupd_curl_get_file_memory(const char *url, struct curl_file_data *file_data);

/**
 * @brief Start a parallel download element.
 *
 * @param max_xfer The maximum number of simultaneos downloads.
 *
 * Parallel download handler will retry max_retries times to download each file,
 * ading a timeout between each try.
 *
 * @note This function is non-blocking.
 */
struct swupd_curl_parallel_handle *swupd_curl_parallel_download_start(size_t max_xfer);

/**
 * @brief Set parallel downloads callbacks.
 *
 * @param success_cb Called for each successful download, where data is the data
 *                   informed on swupd_curl_parallel_download_enqueue().
 *                   success_cb should return true if the download file was
 *                   handled correctly. Return false to schedule a retry.
 * @param error_cb   Called for each failed download, where data is the data
 *                   informed on swupd_curl_parallel_download_enqueue()
 *                   and response is the HTTP response code. error_cb should
 *                   return true if the error was handled by caller or false
 *                   to schedule a retry.
 * @param free_cb    Called when data is ready to be freed.
 */
void swupd_curl_parallel_download_set_callbacks(struct swupd_curl_parallel_handle *handle, swupd_curl_success_cb success_cb, swupd_curl_error_cb error_cb, swupd_curl_free_cb free_cb);

/**
 * @brief Set parallel downloads progress callback
 *
 * @param progress_cb Called periodically by curl while downloading/uploading files.
 *                    The return value of the function is not really important, the
 *                    function prints the download progress, so it will always return 0.
 * @param data        User data to be informed to progress_cb.
 */
void swupd_curl_parallel_download_set_progress_callbacks(struct swupd_curl_parallel_handle *handle, swupd_curl_progress_cb progress_cb, void *data);

/**
 * @brief Enqueue a file to be downloaded. If the number of current downloads is higher
 * than max_xfer, this function will be blocked for downloads until the number of
 * current downloads reach max_xfer / 2.
 *
 * @param handle      Handle created with swupd_curl_parallel_download_start().
 * @param url         The url to be downloaded.
 * @param filename    Full path of the filename to save the download content.
 * @param hash        Optional hex string with hash to be used as unique identifier of this
 *                    file. If NULL, filename will be used as the identifier. String MUST contain
 *                    only characters in '0123456789abcdef'.
 * @param data        User data to be informed to success_cb().
 *
 * @note This function MAY be blocked.
 */
int swupd_curl_parallel_download_enqueue(struct swupd_curl_parallel_handle *handle, const char *url, const char *filename, const char *hash, void *data);

/**
 * @brief Finish all pending downloads and free memory allocated by parallel download
 * handler.
 *
 *  @param handle         Handle created with swupd_curl_parallel_download_start().
 *  @param num_downloads  Optional int pointer to be filled with the number of
 *                        files enqueued for download using this handler. Include
 *                        failed downloads.
 *
 * @note This function MAY be blocked.
 */
int swupd_curl_parallel_download_end(struct swupd_curl_parallel_handle *handle, int *num_downloads);

/**
 * @brief Finish pending tasks for concluded downloads and drop all other pending downloads and clear the download handle.
 *
 *  @param handle         Handle created with swupd_curl_parallel_download_start().
 * @note This function MAY be blocked.
 */
void swupd_curl_parallel_download_cancel(struct swupd_curl_parallel_handle *h);
#ifdef __cplusplus
}
#endif

#endif
