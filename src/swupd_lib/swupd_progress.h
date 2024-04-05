#ifndef __SWUPD_PROGRESS__
#define __SWUPD_PROGRESS__

/**
 * @file
 * @brief Progress callback functions used by fullfile and packs.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "swupd.h"

/**
 * @brief struct to hold information about the overall download progress (including many files).
 */
struct download_progress {
	/** @brief total number of bytes to download. */
	uint64_t total_download_size;
	/** @brief total of bytes downloaded so far. */
	long downloaded;
};

/** @brief struct to hold information about one file download progress. */
struct file_progress {
	/** @brief file size. */
	long file_size;
	/** @brief downloaded. */
	long downloaded;
	/** @brief pointer to a struct holding information about the overall progress. */
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
