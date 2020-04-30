#ifndef __SYSTEM_ROOT__
#define __SYSTEM_ROOT__

/**
 * @file
 * @brief Module that handles installation and removal of files from system root
 */

#include "lib/list.h"
#include "swupd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief Install files from 'files' into the system.
  *
  * @param files The list of files to be installed.
  * @param mom MoM to be used to create missing directories
  */
enum swupd_code target_root_install_files(struct list *files, struct manifest *mom);

/**
  * @brief Install a single file into the system.
  *
  * @param file The file to be installed
  * @param mom MoM to be used to create missing directories
  */
enum swupd_code target_root_install_single_file(struct file *file, struct manifest *mom);

/**
 * @brief Remove from the system all files in list.
 *
 * @param The files to be removed
 */
int target_root_remove_files(struct list *files);

#ifdef __cplusplus
}
#endif

#endif
