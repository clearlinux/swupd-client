#ifndef __FORMATTER_JSON__
#define __FORMATTER_JSON__

/**
 * @file
 * @brief Format the output to print using JSON format.
 */

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enables the JSON formatter
 */
void set_json_format(void);

/**
 * @brief Generates the initial message of a JSON stream
 */
void json_start(const char *);

/**
 * @brief Generates the final message of a JSON stream
 */
void json_end(const char *, int);

/**
 * @brief Reports the status of an operation into the JSON stream
 */
void json_status(int);

/**
 * @brief Converts the provided message to a JSON stream
 */
void json_message(const char *, const char *, va_list);

/**
 * @brief Prints the progress of a given step into the JSON stream
 */
void json_progress(char *, unsigned int, unsigned int, int);

#ifdef __cplusplus
}
#endif
#endif
