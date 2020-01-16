#ifndef __COMP_FUNCTIONS_H
#define __COMP_FUNCTIONS_H

/**
 * @file
 * @brief Compare like functions.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Definition of a funtion type to compare two elements
 */
typedef int (*comparison_fn_t)(const void *a, const void *b);

/**
 * @brief strcmp wrapper casting void * to char *.
 */
int strcmp_wrapper(const void *a, const void *b);

#ifdef __cplusplus
}
#endif
#endif
