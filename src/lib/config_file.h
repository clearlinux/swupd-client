#ifndef __CONFIG_FILE__
#define __CONFIG_FILE__

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
typedef bool (*parse_config_fn_t)(char *section, char *key, char *value, void *data);

/**
 * @brief Parses the INI style configuration file and sets up the runtime environment
 * based on the configuration values
 * @param filename: the full path to the configuration file to parse
 * @param parse_config_fn: a pointer to a function that takes specific actions based
 * on the key/value pairs
 * @param data: user's data to be used on parse_config_fn() calls.
 *
 * @returns True if there were no errors when parsing and processing the parsed options,
 *          False otherwise
 */
bool config_file_parse(const char *filename, parse_config_fn_t parse_config_fn, void *data);

/**
 * @brief Writes a section to ini file using string as an argument.
 *
 * @param FILE file pointer *FILE to the config file
 * @param section string to written to config file
 * @returns 0 if no errors or negative on any error
 */
int config_write_section(FILE *file, const char *section);

/**
 * @brief Writes a config(key,value) line to ini file using key,value strings as an
 * argument
 *
 * @param FILE file pointer *FILE to the config file
 * @param key string
 * @param value string
 * @returns 0 if no errors or negative on any error
 */
int config_write_config(FILE *file, const char *key, const char *value);

#ifdef __cplusplus
}
#endif
#endif
