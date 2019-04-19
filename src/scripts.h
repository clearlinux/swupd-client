#ifndef __SCRIPTS__
#define __SCRIPTS__

/**
 * @file
 * @brief Scripts execution functions.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run post-update scripts.
 *
 * @param block If true wait for the script to finish before returning.
 */
void scripts_run_post_update(bool block);

/**
 * @brief Run pre-update scripts.
 *
 * @param manifests MoM that includes the pre-update script to be run.
 *
 * Pre update scripts are executed only if they are found and if the hash
 * matches. In the case of failures, update will continue.
 */
void scripts_run_pre_update(struct manifest *manifest);

#ifdef __cplusplus
}
#endif

#endif
