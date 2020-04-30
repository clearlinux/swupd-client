#ifndef __VERIFYTIME__
#define __VERIFYTIME__

/**
 * @file
 * @brief Verify and correct sytem time if incorrect.
 */

#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Verify system time and correct if needed.
 *
 * @param path_prefix The path where the system is installed.
 * @param system_time To be set with the current time
 * @param clear_time To be set with the timestamp of the clear release
 *
 * Function uses timestamp in system path to check if the current system time
 * diverged. If so, update current system time with timestamp. If timestamp is
 * not found in path_prefix, file is also looked on '/'.
 *
 * @return 0 if the time is correct, -ENOENT if the timestamp in the system is
 * not available and -ETIME if time is out of sync.
 */
int verify_time(char *path_prefix, time_t *system_time, time_t *clear_time);

/**
 * @brief Helper Convert time_t to a human readable string.
 */
char *time_to_string(time_t t);

/**
 * @brief Set system time to mtime
 */
bool set_time(time_t mtime);

#ifdef __cplusplus
}
#endif

#endif
