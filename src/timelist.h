#ifndef __TIME_H__
#define __TIME_H__

#include <sys/queue.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef TAILQ_HEAD(timelist, time) timelist;

/* Create a new timelist that should be freed with timelist_free() */
timelist *timelist_new(void);

/* Start recording time of a task and append that information in the timelist.
 * Time recording should be stopped using timelist_timer_stop(). */
void timelist_timer_start(timelist *list, const char *name);

/* Stop recording time for last running task in timelist */
void timelist_timer_stop(timelist *list);

/* Print time statistics recorded using timelist_timer_start and timelist_timer_stop */
void timelist_print_stats(timelist *list);

/* Free timelist and all data from the list */
void timelist_free(timelist *head);

#ifdef __cplusplus
}
#endif

#endif
