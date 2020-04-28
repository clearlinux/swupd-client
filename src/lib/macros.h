#ifndef __INCLUDE_GUARD_MACROS_H
#define __INCLUDE_GUARD_MACROS_H

#include <stdio.h>
#include <stdlib.h>
/**
 * @file
 * @brief General use macros
 */

/** @brief Abort if var is NULL. */
#define ON_NULL_ABORT(var) \
	if (!var) {        \
		abort();   \
	}

/** @brief Set parameter as unused. */
#define UNUSED_PARAM __attribute__((__unused__))

/** @brief Return the max between 2 values. */
#define MAX(_a, _b) ((_a) > (_b) ? (_a) : (_b))

#ifdef DEBUG_MODE
#define UNEXPECTED()                                                                   \
	do {                                                                           \
		fprintf(stderr, "Unexpected condition (%s:%d)\n", __FILE__, __LINE__); \
		abort();                                                               \
	} while (0)
#else
/**
 * @brief To be used in all checks where we assume swupd should never get in to.
 * This macro aborts execution only when DEBUG_MODE is used.
 */
#define UNEXPECTED()                                                                                         \
	do {                                                                                                 \
		telemetry(TELEMETRY_CRIT, "unexpected-condition", "file=%s\nline=%d\n", __FILE__, __LINE__); \
	} while (0)

#endif

/**
 * @brief free() wrapper that always set the pointer to NULL after freeing.
 * This macro should be used instead of free() to prevent invalid memory access
 */
#define FREE(_ptr)           \
	do {                 \
		free(_ptr);  \
		_ptr = NULL; \
	} while (0)

/**
 * @brief allocate memory of indicated size and zero it.
 *
 * Abort on out of memory errors.
 */
static inline void *malloc_or_die(size_t size)
{
	void *ptr;

	ptr = calloc(1, (size_t)size);
	ON_NULL_ABORT(ptr);

	return ptr;
}

#endif
