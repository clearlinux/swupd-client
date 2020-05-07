#ifndef __STATEDIR__
#define __STATEDIR__

/**
 * @file
 * @brief Module that handles the statedir
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief Gets the path to the bundle tracking directory in the statedir.
  */
char *statedir_get_tracking_dir(void);

/**
 * @brief Gets the path to the tracking file of the specified bundle in the statedir.
 *
 * @param bundle_name, the name of the bundle
 */
char *statedir_get_tracking_file(const char *bundle_name);

/**
  * @brief Gets the path to the staged directory in the statedir.
  */
char *statedir_get_staged_dir(void);

/**
 * @brief Gets the path to a file in the staged directory in the statedir.
 *
 * @param file_hash, the hash of the file
 */
char *statedir_get_staged_file(char *file_hash);

#ifdef __cplusplus
}
#endif

#endif
