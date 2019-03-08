#ifndef __SWUPD_LOG__
#define __SWUPD_LOG__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_ALWAYS 0
#define LOG_ERROR 2
#define LOG_WARN 4
#define LOG_INFO 6
#define LOG_DEBUG 8

/*
 * Set the minimum priority log to be printed by log functions.
 */
void set_log_level(int log_level);

/*
 * Print a message to 'out' if 'log_level' is greater or equal the log level
 * set by set_log_level().
 */
void log_full(int log_level, FILE *out, const char *file, int line, const char *label, const char *format, ...);

/*
 * Helpers for logging: print(), error(), warn(), info(), debug()
 */
#define print(_fmt, ...) log_full(LOG_ALWAYS, stdout, __FILE__, __LINE__, NULL, _fmt, ##__VA_ARGS__);
#define error(_fmt, ...) log_full(LOG_ERROR, stderr, __FILE__, __LINE__, "Error", _fmt, ##__VA_ARGS__);
#define warn(_fmt, ...) log_full(LOG_WARN, stderr, __FILE__, __LINE__, "Warning", _fmt, ##__VA_ARGS__);
#define info(_fmt, ...) log_full(LOG_INFO, stdout, __FILE__, __LINE__, NULL, _fmt, ##__VA_ARGS__);
#define debug(_fmt, ...) log_full(LOG_DEBUG, stdout, __FILE__, __LINE__, "Debug", _fmt, ##__VA_ARGS__);

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

#ifdef __cplusplus
}
#endif
#endif
