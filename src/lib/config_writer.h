#ifndef __CONFIG_WRITER__
#define __CONFIG_WRITER__

/**
 * @file
 * @brief Configuration fie writing
 */

#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

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
