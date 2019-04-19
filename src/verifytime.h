#ifndef __VERIFYTIME__
#define __VERIFYTIME__

/**
 * @file
 * @brief Verify and correct sytem time if incorrect.
 */

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Verify system time and correct if needed.
 * @param path_prefix The path where the system is installed.
 *
 * Function uses timestamp in system path to check if the current system time
 * diverged. If so, update current system time with timestamp. If timestamp is
 * not found in path_prefix, file is also looked on '/'.
 */
bool verify_time(char *path_prefix);

#ifdef __cplusplus
}
#endif

#endif
