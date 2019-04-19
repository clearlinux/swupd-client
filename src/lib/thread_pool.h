#ifndef __THREAD_POOL__
#define __THREAD_POOL__

/**
 * @file
 * @brief Really simple and small thread pool implementation for swupd.
 */

#include <stdlib.h>

/** @brief Internal information about the thread pool. */
struct tp;

/**
 * Thread task function type definition.
 */
typedef void (*tp_task_run_t)(void *data);

/**
 * Create a new thread pool.
 *
 * @param num_threads The number of threads to be created.
 *
 * @note Free thread pool data with tp_free()
 */
struct tp *tp_start(int num_threads);

/**
 * @brief Schedule a task to be run in this thread pool.
 *
 * @param tp The thread pool
 * @param run Callback to be executed in a new thread.
 * @param data Data informed to callback.
 */
int tp_task_schedule(struct tp *tp, tp_task_run_t run, void *data);

/**
 * @brief Wait for all scheduled tasks to be completed, finishes all threads and
 * release all memory used by the thread pool.
 */
void tp_complete(struct tp *tp);

/**
 * @brief Get the number of threads created by this thread pool.
 */
int tp_get_num_threads(struct tp *tp);

//TODO: Implement a tp_wait() function

#endif
