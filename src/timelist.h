#ifndef __TIME_H__
#define __TIME_H__

#ifdef __cplusplus
extern "C" {
#endif

struct timelist;

/* Create a new timelist that should be freed with timelist_free() */
struct timelist *timelist_new(void);

/* Start recording time of a task and append that information in the timelist.
 * Time recording should be stopped using timelist_timer_stop(). */
void timelist_timer_start(struct timelist *list, const char *name);

/* Stop recording time for last running task in timelist */
void timelist_timer_stop(struct timelist *list);

/* Print time statistics recorded using timelist_timer_start and timelist_timer_stop */
void timelist_print_stats(struct timelist *list);

/* Free timelist and all data from the list */
void timelist_free(struct timelist *head);

#ifdef __cplusplus
}
#endif

#endif
