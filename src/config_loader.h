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

/**
 * @brief Type of function that sets default values for long options
 */
typedef bool (*long_opt_default_fn_t)(const char *long_opt);

#ifdef __cplusplus
}
#endif
#endif
