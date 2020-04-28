#ifndef SWUPD_INT_H
#define SWUPD_INT_H

/**
 * @file
 * @brief Basic string operations.
 */

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convert from ssize_t to size_t.
 *
 * @returns 0 on success and negative int on errors:
 * - If ssize_t param is negative, returns -ERANGE
 * - If b is NULL, return -EINVAL
 */
int ssize_to_size_err(ssize_t a, size_t *b);

/**
 * @brief Convert from int to unsigned int.
 *
 * @returns 0 on success and negative int on errors:
 * - If ssize_t param is negative, returns -ERANGE
 * - If b is NULL, return -EINVAL
 */
int int_to_uint_err(int a, unsigned int *b);

/**
 * @brief Convert from int to unsigned int.
 *
 * @returns The value in an unsigned int format or 0 on negative or errors.
 */
unsigned int int_to_uint(int a);

/**
 * @brief Convert from ssize_t to size_t.
 *
 * @returns The value in a size_t format or 0 on negative or errors.
 */
size_t ssize_to_size(int a);

#ifdef __cplusplus
}
#endif

#endif
