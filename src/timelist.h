#ifndef __TIME_H__
#define __TIME_H__

#include <sys/queue.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef TAILQ_HEAD(timelist, time) timelist;

extern timelist init_timelist(void);
extern void grabtime_start(timelist *list, const char *name);
extern void grabtime_stop(timelist *list);
extern void print_time_stats(timelist *list);

#ifdef __cplusplus
}
#endif

#endif
