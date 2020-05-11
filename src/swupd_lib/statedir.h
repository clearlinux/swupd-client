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

/**
  * @brief Gets the path to the delta directory in the statedir.
  */
char *statedir_get_delta_dir(void);

/**
  * @brief Gets the path to the download directory in the statedir.
  */
char *statedir_get_download_dir(void);

/**
 * @brief Gets the path to the downloaded fullfile tar in the statedir.
 *
 * @param file_hash, the hash of the file
 */
char *statedir_get_fullfile_tar(char *file_hash);

/**
 * @brief Gets the path to the downloaded manifest tar of the specified
 * component at a certain version in the statedir.
 *
 * @param version, the version of the manifest
 * @param component, either MoM or the name of a bundle
 */
char *statedir_get_manifest_tar(int version, char *component);

/**
 * @brief Gets the path to the manifest of the specified component at a
 * certain version in the statedir.
 *
 * @param version, the version of the manifest
 * @param component, either MoM or the name of a bundle
 */
char *statedir_get_manifest(int version, char *component);

/**
 * @brief Gets the path to the manifest that contains its own hash
 * of the specified component at a certain version in the statedir.
 *
 * @param version, the version of the manifest
 * @param component, either MoM or the name of a bundle
 * @param manifest_hash, the hash of the manifest
 */
char *statedir_get_hashed_manifest(int version, char *component, char *manifest_hash);

/**
 * @brief Gets the path to the temporary name given to a downloaded
 * fullfile tar while being untarred in the statedir.
 *
 * @param file_hash, the hash of the file
 */
char *statedir_get_fullfile_renamed_tar(char *file_hash);

/**
 * @brief Gets the path to the telemetry record in the statedir.
 *
 * @param record, the name of the telemetry record
 */
char *statedir_get_telemetry_record(char *record);

/**
 * @brief Creates the required directories in the statedir.
 *
 * @param path The path of the statedir
 */
int statedir_create_dirs(const char *path);

#ifdef __cplusplus
}
#endif

#endif
