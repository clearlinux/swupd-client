#ifndef __FORMATTER_JSON__
#define __FORMATTER_JSON__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UNUSED __attribute__((__unused__))
#define JSON_FORMAT 1

/*
 * Enables the JSON formatter
 */
void set_json_format(void);

/*
 * Generates the initial message of a JSON stream
 */
void json_start(const char *);

/*
 * Generates the final message of a JSON stream
 */
void json_end(const char *, int);

/*
 * Reports the status of an operation into the JSON stream
 */
void json_status(int);

/*
 * Converts the provided message to a JSON stream
 */
void json_message(const char *, const char *, va_list);

/*
 * Prints the progress of a given step into the JSON stream
 */
void json_progress(char *, unsigned int, unsigned int, int);

#ifdef __cplusplus
}
#endif
#endif
