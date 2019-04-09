#ifndef __SCRIPTS__
#define __SCRIPTS__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Run post-update scripts.
 */
void scripts_run_post_update(bool block);

/*
 * Run pre-update scripts.
 *
 * 'manifests' should point the MoM that includes the pre-update script to be
 * run.
 *
 * Pre update scripts are executed only if they are found and if the hash
 * matches. In the case of failures, update will continue.
 *
 */
void scripts_run_pre_update(struct manifest *manifest);

#ifdef __cplusplus
}
#endif

#endif
