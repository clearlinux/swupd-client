#ifndef SWUPD_INT_H
#define SWUPD_INT_H

/**
 * @file
 * @brief Basic int operations.
 */

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convert from long to unsigned long.
 *
 * @returns 0 on success and negative int on errors:
 * - If long param is negative, returns -ERANGE
 * - If b is NULL, return -EINVAL
 */
int long_to_ulong_err(long a, unsigned long *b);

/**
 * @brief Convert from int to unsigned int.
 *
 * @returns 0 on success and negative int on errors:
 * - If int param is negative, returns -ERANGE
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
 * @brief Convert from long to unsigned long.
 *
 * @returns The value in a unsigned long format or 0 on negative or errors.
 */
unsigned long long_to_ulong(long a);

/**
 * @brief Convert from unsigned int to int.
 *
 * @returns 0 on success and negative int on errors:
 * - If the operation overflows, returns -ERANGE
 * - If b is NULL, return -EINVAL
 */
int uint_to_int_err(unsigned int a, int *b);

/**
 * @brief Convert from unsigned int to int.
 *
 * @returns The value in int format or INT_MAX on overflows.
 */
int uint_to_int(unsigned int a);

/**
 * @brief Convert from unsigned long to long
 *
 * @returns 0 on success and negative int on errors:
 * - If the operation overflows, returns -ERANGE
 * - If b is NULL, return -EINVAL
 */
int ulong_to_long_err(unsigned long a, long *b);

/**
 * @brief Convert from unsigned long to long
 *
 * @returns The value in int format or LONG_MAX on overflows.
 */
long ulong_to_long(unsigned long a);

/**
 * @brief Convert from unsigned long to int
 *
 * @returns 0 on success and negative int on errors:
 * - If the operation overflows, returns -ERANGE
 * - If b is NULL, return -EINVAL
 */
int ulong_to_int_err(unsigned long a, int *b);

/**
 * @brief Convert from unsigned long to int
 *
 * @returns The value in int format or INT_MAX on overflows.
 */
int ulong_to_int(unsigned long a);

/**
 * @brief Convert from long to int
 *
 * @returns 0 on success and negative int on errors:
 * - If the operation overflows, returns -ERANGE
 * - If b is NULL, return -EINVAL
 */
int long_to_int_err(long a, int *b);

/**
 * @brief Convert from long to int
 *
 * @returns The value in int format, INT_MAX on overflows or INT_MIN on
 * underflows
 */
int long_to_int(long a);
#ifdef __cplusplus
}
#endif

#endif
