#ifndef __SWUPD_PROGRESS__
#define __SWUPD_PROGRESS__

/**
 * @file
 * @brief Progress callback functions used by fullfile and packs.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define UNUSED_PARAM __attribute__((__unused__))

#include <curl/curl.h>

/* struct to hold information about the overall download progress (including many files) */
struct download_progress {
	long total_download_size; /* total number of bytes to download */
	long downloaded;	  /* total of bytes downloaded so far */
};

/* struct to hold information about one file download progress */
struct file_progress {
	long file_size;
	long downloaded;
	struct download_progress *overall_progress;
};

/**
 * @brief Callback function called periodically by CURL to report how many bytes
 * it has downloaded.
 */
int swupd_progress_callback(void *clientp, int64_t dltotal, int64_t dlnow, int64_t UNUSED_PARAM ultotal, int64_t UNUSED_PARAM ulnow);

#ifdef __cplusplus
}
#endif
#endif
