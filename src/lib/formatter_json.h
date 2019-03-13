#ifndef __FORMATTER_JSON__
#define __FORMATTER_JSON__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UNUSED __attribute__((__unused__))

/*
 * Enables the JSON formatter
 */
void set_json_format(void);

/*
 * Converts the provided message to a JSON formatted stream
 */
void format_to_json(const char *, const char *, va_list);

/*
 * Generates the initial message of a JSON stream
 */
void json_start(const char *);

/*
 * Generates the final message of a JSON stream
 */
void json_end(const char *);

/*
 * Reports the status of an operation into the JSON stream
 */
void json_status(int);

#ifdef __cplusplus
}
#endif
#endif
