#ifndef __INCLUDE_GUARD_PROGRESS_H
#define __INCLUDE_GUARD_PROGRESS_H


#ifdef __cplusplus
extern "C" {
#endif

void progress_start(char *header, int total);
void progress_update(int current, const char *partial_text);
void progress_end();

#ifdef __cplusplus
}
#endif

#endif
