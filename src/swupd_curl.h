#ifndef __SWUPD_CURL__
#define __SWUPD_CURL__

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

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

typedef bool (*swupd_curl_success_cb)(void *data);
typedef bool (*swupd_curl_error_cb)(enum download_status status, void *data);
typedef void (*swupd_curl_free_cb)(void *data);

int swupd_curl_init(void);
void swupd_curl_deinit(void);
double swupd_curl_query_content_size(char *url);
int swupd_curl_get_file(const char *url, char *filename);
int swupd_curl_get_file_memory(const char *url, struct curl_file_data *file_data);

void *swupd_curl_parallel_download_start(size_t max_xfer);
void swupd_curl_parallel_download_set_callbacks(void *handle, swupd_curl_success_cb success_cb, swupd_curl_error_cb error_cb, swupd_curl_free_cb free_cb);
int swupd_curl_parallel_download_enqueue(void *handle, const char *url, const char *filename, const char *hash, void *data);
int swupd_curl_parallel_download_end(void *handle, int *num_downloads);

#ifdef __cplusplus
}
#endif

#endif
