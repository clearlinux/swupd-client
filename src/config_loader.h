#ifndef __CONFIG_LOADER__
#define __CONFIG_LOADER__

/**
 * @file
 * @brief Load options from a configuration file
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @brief Type of function that sets options for a command based on parsed values
 */
typedef bool (*parse_opt_fn_t)(int opt, char *optarg);

#ifdef __cplusplus
}
#endif
#endif
