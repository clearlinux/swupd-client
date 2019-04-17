#ifndef __SWUPD_PROGRESS__
#define __SWUPD_PROGRESS__

#ifdef __cplusplus
extern "C" {
#endif

#define UNUSED_PARAM __attribute__((__unused__))

#include <curl/curl.h>

struct download_progress {
	double total_download_size; /* total number of bytes to download */
	double current;		    /* number of bytes that has been already downloaded */
	double dlprev;		    /* previous download read provided by curl */
};

/*
 * Callback function called periodically by CURL to report how many bytes
 * it has downloaded.
 */
int swupd_progress_callback(void *clientp, int64_t dltotal, int64_t dlnow, int64_t UNUSED_PARAM ultotal, int64_t UNUSED_PARAM ulnow);

#ifdef __cplusplus
}
#endif
#endif
