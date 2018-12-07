#ifndef SWUPD_STRINGS_H
#define SWUPD_STRINGS_H

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Save in strp a new allocated string with content printed from fmt and
 * parameters using vasprintf. Abort on memory allocation errors.
 */
void string_or_die(char **strp, const char *fmt, ...);

/* Return a duplicated copy of the string using strdup().
 * Abort if there's no memory to allocate the new string.
 */
char *strdup_or_die(const char *const str);

/* Free string and set it's value to NULL */
void free_string(char **s);

/* Join strings from string list separated by the separator */
char *string_join(const char *separator, struct list *string);

#ifdef __cplusplus
}
#endif

#endif
