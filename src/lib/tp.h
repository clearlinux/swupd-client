#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <stdlib.h>

/*
 * Really simple and small thread pool implementation for swupd.
 */

struct tp;

/*
 * Thread task function type definition.
 */
typedef void (*tp_task_run_t)(void *data);

/*
 * Create a new thread pool.
 *
 * num_threads: The number of threads to be created.
 *
 * Note: Free thread pool data with tp_free()
 */
struct tp *tp_start(int num_threads);

/*
 * Schedule a task to be run in this thread pool.
 *
 * run: Callback to be executed in a new thread.
 * data: Data informed to callback.
 */
int tp_task_schedule(struct tp *tp, tp_task_run_t run, void *data);

/*
 * Wait for all scheduled tasks to be completed, finishes all threads and
 * release all memory used by the thread pool.
 */
void tp_complete(struct tp *tp);

/*
 * Get number of CPU cores.
 */
int tp_get_num_cores();

//TODO: Implement a tp_wait() function

#endif
