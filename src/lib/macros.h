#ifndef __INCLUDE_GUARD_MACROS_H
#define __INCLUDE_GUARD_MACROS_H

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
#define UNEXPECTED() abort()
#else
/**
 * @brief To be used in all checks where we assume swupd should never get in to.
 * This macro aborts execution only when DEBUG_MODE is used.
 */
#define UNEXPECTED()
#endif

#endif
