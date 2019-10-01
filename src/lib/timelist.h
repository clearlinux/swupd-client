#ifndef __TIME_H__
#define __TIME_H__

/**
 * @file
 * @brief Measure time functions take to execute.
 */

#include <sys/queue.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief A timelist object
 */
typedef TAILQ_HEAD(timelist, time) timelist;

/**
 * @brief Create a new timelist that should be freed with timelist_free()
 */
timelist *timelist_new(void);

/**
 * @brief Start recording time of a task and append that information in the timelist.
 *
 * Time recording should be stopped using timelist_timer_stop().
 */
void timelist_timer_start(timelist *list, const char *name);

/**
 * @brief Stop recording time for last running task in timelist
 */
void timelist_timer_stop(timelist *list);

/**
 * @brief Print time statistics recorded using timelist_timer_start and timelist_timer_stop
 */
void timelist_print_stats(timelist *list);

/**
 * @brief Free timelist and all data from the list
 */
void timelist_free(timelist *head);

#ifdef __cplusplus
}
#endif

#endif
