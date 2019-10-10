#ifndef __FORMATTER_JSON__
#define __FORMATTER_JSON__

/**
 * @file
 * @brief Format the output to print using JSON format.
 */

#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generates the initial message of a JSON stream
 */
void json_start(const char *);

/**
 * @brief Generates the final message of a JSON stream
 */
void json_end(const char *, int);

/**
 * @brief Log message using Json format
 */
void log_json(FILE *out, const char *file, int line, const char *label, const char *format, va_list args_list);

/**
 * @brief Prints the progress of a given step into the JSON stream
 */
void json_progress(const char *, unsigned int, unsigned int, int);

#ifdef __cplusplus
}
#endif
#endif
