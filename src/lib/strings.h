#ifndef SWUPD_STRINGS_H
#define SWUPD_STRINGS_H

/**
 * @file
 * @brief Basic string operations.
 */

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Save in strp a new allocated string with content printed from fmt and
 * parameters using vasprintf. Abort on memory allocation errors.
 */
void string_or_die(char **strp, const char *fmt, ...);

/**
 * Return a new allocated string with the content printed from fmt and
 * parameters using vasprintf. Abort on memory allocation errors.
 *
 * Similar to string_or_die(), but returns the string pointer.
 */
char *str_or_die(const char *fmt, ...);

/**
 * Return a duplicated copy of the string using strdup().
 * Abort if there's no memory to allocate the new string.
 */
char *strdup_or_die(const char *const str);

/**
 * Free string and set it's value to NULL.
 */
void free_string(char **s);

/**
 * Join strings from string list separated by the separator.
 */
char *string_join(const char *separator, struct list *string);

/**
 * Safely convert string to integer avoiding overflows
 *
 * The strtol function is commonly used to convert a string to a number and
 * the result is frequently stored in an int type, but type casting a long to
 * an int can cause overflows.
 *
 * This function returns negative error codes based on the errno table:
 * -ERANGE is returned when the string is out of range for int value
 * -EINVAL is returned when the string isn't a valid number or has any invalid
 * trailing character.
 */
int strtoi_err(const char *str, int *value);

/**
 * Safely convert and string to integer avoiding overflows.
 *
 * The strtol function is commonly used to convert a string to a number and
 * the result is frequently stored in an int type, but type casting a long to
 * an int can cause overflows.
 *
 * This function returns negative error codes based on the errno table:
 * -ERANGE is returned when the string is out of range for int value
 *
 * endptr is set with the value of the first invalid character in the string.
 */
int strtoi_err_endptr(const char *str, char **endptr, int *value);

/**
 * Creates a new string converting all characters from the original string
 * to its lower case values.
 *
 * This function returns a pointer to the new string.
 *
 * The memory of the new string has to be freed after using it.
 */
char *str_tolower(const char *str);

/**
 * Converts a string to a boolean. If the string is "true" then it is converted
 * to a boolean true. Everything else is converted to a boolean false.
 */
bool strtobool(const char *str);

#ifdef __cplusplus
}
#endif

#endif
