#ifndef __CONFIG_PARSER__
#define __CONFIG_PARSER__

/**
 * @file
 * @brief Configuration file parsing
 */

#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function type to process the parsed key/values
 */
typedef bool (*load_config_fn_t)(char *section, char *key, char *value);

/**
 * @brief Parses the INI style configuration file and sets up the runtime environment
 * based on the configuration values
 * @param filename: the full path to the configuration file to parse
 * @param load_config_fn: a pointer to a function that takes specific actions based
 * on the key/value pairs
 *
 * @returns True if there were no errors when parsing and processing the parsed options,
 *          False otherwise
 */
bool config_parse(const char *filename, load_config_fn_t load_config_fn);

#ifdef __cplusplus
}
#endif
#endif
