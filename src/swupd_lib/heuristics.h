#ifndef __HEURISTICS__
#define __HEURISTICS__

/**
 * @file
 * @brief Heuristics system to define which scripts to run and which files to
 * ignore on installation time.
 */

#include <stdbool.h>
#include <stdint.h>

#include "swupd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Use heuristics to define which files should be updated and which
 * scripts should be executed.
 *
 * Use the path and properties of the file to define it's type and by that
 * define if the file should be set with a do_not_update flag or is_boot flag.
 * Also use that information to check if the bootloader or systemd update
 * scripts should be executed.
 *
 * @param files The list of struct file objects where swupd heuristics will be
 * applied.
 */
void heuristics_apply(struct list *files);

#ifdef __cplusplus
}
#endif

#endif
