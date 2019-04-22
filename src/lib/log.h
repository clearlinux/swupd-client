#ifndef __SWUPD_LOG__
#define __SWUPD_LOG__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief Log functions.
 */

/** @brief Only show messages printed with print(). */
#define LOG_ALWAYS 0
/** @brief Only show messages printed error() and print(). */
#define LOG_ERROR 2
/** @brief Only show messages printed warn(), error() and print(). */
#define LOG_WARN 4
/** @brief Only show messages printed info(), warn(), error() and print(). */
#define LOG_INFO 6
/** @brief Print all messages. */
#define LOG_DEBUG 8

/**
 * @brief Log callback functions.
 * @see log_set_function()
 */
typedef void (*log_fn_t)(FILE *out, const char *file, int line, const char *label, const char *format, va_list args_list);

/**
 * @brief Set the log function to use. Replaces the default (printf).
 */
void log_set_function(log_fn_t log_fn);

/**
 * @brief Set the minimum priority log to be printed by log functions.
 */
void log_set_level(int log_level);

/**
 * @brief Print a message to 'out' if 'log_level' is greater or equal the log level
 * set by log_set_level().
 */
void log_full(int log_level, FILE *out, const char *file, int line, const char *label, const char *format, ...);

/** @brief Print messages using LOG_ALWAYS level. */
#define print(_fmt, ...) log_full(LOG_ALWAYS, stdout, __FILE__, __LINE__, NULL, _fmt, ##__VA_ARGS__);
/** @brief Print messages using LOG_ERROR level. */
#define error(_fmt, ...) log_full(LOG_ERROR, stderr, __FILE__, __LINE__, "Error", _fmt, ##__VA_ARGS__);
/** @brief Print messages using LOG_WARN level. */
#define warn(_fmt, ...) log_full(LOG_WARN, stderr, __FILE__, __LINE__, "Warning", _fmt, ##__VA_ARGS__);
/** @brief Print messages using LOG_INFO level. */
#define info(_fmt, ...) log_full(LOG_INFO, stdout, __FILE__, __LINE__, NULL, _fmt, ##__VA_ARGS__);
/** @brief Print messages using LOG_DEBUG level. */
#define debug(_fmt, ...) log_full(LOG_DEBUG, stdout, __FILE__, __LINE__, "Debug", _fmt, ##__VA_ARGS__);

#ifdef __cplusplus
}
#endif
#endif
