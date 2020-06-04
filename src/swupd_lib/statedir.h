#ifndef __STATEDIR__
#define __STATEDIR__

/**
 * @file
 * @brief Module that handles the statedir
 */

#ifdef __cplusplus
extern "C" {
#endif

/* ******************** */
/* Data (state) section */
/* ******************** */

/**
  * @brief Gets the path to the bundle tracking directory in the datadir.
  */
char *statedir_get_tracking_dir(void);

/**
 * @brief Gets the path to the tracking file of the specified bundle in the datadir.
 *
 * @param bundle_name, the name of the bundle
 */
char *statedir_get_tracking_file(const char *bundle_name);

/**
 * @brief Gets the path to the telemetry record in the datadir.
 *
 * @param record, the name of the telemetry record
 */
char *statedir_get_telemetry_record(char *record);

/**
 * @brief Gets the path to the swupd lock file in the datadir.
 */
char *statedir_get_swupd_lock(void);

/**
 * @brief Gets the path to the version file in the datadir.
 */
char *statedir_get_version(void);


/* ************* */
/* Cache section */
/* ************* */

/**
 * @brief Gets the path to the root cache directory in the cachedir.
 */
char *statedir_get_cache_dir(void);

/**
 * @brief Gets the path to the url specific cache directory in the cachedir.
 */
char *statedir_get_cache_url_dir(void);

/**
  * @brief Gets the path to the delta directory in the cachedir.
  */
char *statedir_get_delta_dir(void);

/**
 * @brief Gets the path to the directory where delta-packs are
 * stored in the cachedir.
 */
char *statedir_get_delta_pack_dir(void);

/**
 * @brief Gets the path to the delta pack tar of the specified
 * bundle in the cachedir.
 *
 * @param bundle, the name of a bundle
 * @param from_version, the from version for the delta
 * @param to_version, the to version for the delta
 */
char *statedir_get_delta_pack(char *bundle, int from_version, int to_version);

/**
  * @brief Gets the path to the download directory in the cachedir.
  */
char *statedir_get_download_dir(void);

/**
  * @brief Gets the path to a downloaded file in the cachedir.
  * @param filename, the name of the file to get
  */
char *statedir_get_download_file(char * filename);

/**
 * @brief Gets the path to the directory where manifests are organized
 * in the cachedir.
 */
char *statedir_get_manifest_root_dir(void);

/**
 * @brief Gets the path to the directory where manifests for a specific
 * version are stored in the cachedir.
 *
 * @param version, the version of the manifests directory
 */
char *statedir_get_manifest_dir(int version);

/**
 * @brief Gets the path to the directory where manifests for a specific
 * version are stored in the cachedir duplicate
 * (formerly known as statedir_cache).
 *
 * @param version, the version of the manifests directory
 */
char *statedir_dup_get_manifest_dir(int version);

/**
 * @brief Gets the path to the manifest of the specified component
 * in the cachedir.
 *
 * @param version, the version of the manifest
 * @param component, either MoM or the name of a bundle
 */
char *statedir_get_manifest(int version, char *component);

/**
 * @brief Gets the path to the manifest of the specified component
 * in the cachedir duplicate (formerly known as statedir_cache).
 *
 * @param version, the version of the manifest
 * @param component, either MoM or the name of a bundle
 */
char *statedir_dup_get_manifest(int version, char *component);

/**
  * @brief Gets the path to the staged directory in the cachedir.
  */
char *statedir_get_staged_dir(void);

/**
 * @brief Gets the path to a file in the staged directory in the cachedir.
 *
 * @param file_hash, the hash of the file
 */
char *statedir_get_staged_file(char *file_hash);

/**
 * @brief Gets the path to a file in the staged directory in the
 * cachedir duplicate (formerly known as statedir_cache).
 *
 * @param file_hash, the hash of the file
 */
char *statedir_dup_get_staged_file(char *file_hash);

/**
 * @brief Gets the path to the downloaded fullfile tar in the cachedir.
 *
 * @param file_hash, the hash of the file
 */
char *statedir_get_fullfile_tar(char *file_hash);

/**
 * @brief Gets the path to the downloaded manifest tar of the specified
 * component in the cachedir.
 *
 * @param version, the version of the manifest
 * @param component, either MoM or the name of a bundle
 */
char *statedir_get_manifest_tar(int version, char *component);

/**
 * @brief Gets the path to the manifest that contains its own hash
 * of the specified component in the cachedir.
 *
 * @param version, the version of the manifest
 * @param component, either MoM or the name of a bundle
 * @param manifest_hash, the hash of the manifest
 */
char *statedir_get_manifest_with_hash(int version, char *component, char *manifest_hash);

/**
  * @brief Gets the path to the manifest delta directory in the cachedir.
  */
char *statedir_get_manifest_delta_dir(void);

/**
 * @brief Gets the path to the manifest delta of the specified bundle
 * going from one version to another version in the cachedir.
 *
 * @param bundle, the name of a bundle
 * @param from_version, the from version for the delta
 * @param to_version, the to version for the delta
 */
char *statedir_get_manifest_delta(char *bundle, int from_version, int to_version);

/**
 * @brief Gets the path to the temporary name given to a downloaded
 * fullfile tar while being untarred in the cachedir.
 *
 * @param file_hash, the hash of the file
 */
char *statedir_get_fullfile_renamed_tar(char *file_hash);

/**
 * @brief Gets the path to the delta pack tar of the specified bundle
 * going from one version to another version in the cachedir duplicate
 * (formerly known as statedir_cache).
 *
 * @param bundle, the name of a bundle
 * @param from_version, the from version for the delta
 * @param to_version, the to version for the delta
 */
char *statedir_dup_get_delta_pack(char *bundle, int from_version, int to_version);

/**
 * @brief Gets the path to the temporary directory in the cachedir.
 */
char *statedir_get_temp_dir(void);

/**
 * @brief Gets the path to the temporary file in the cachedir.
 *
 * @param filename, the name of the file
 */
char *statedir_get_temp_file(char *filename);

/* *************** */
/* Setup functions */
/* *************** */

/**
 * @brief Creates the required directories in the datadir and cachedir.
 *
  * @param include_all If set to true, all directories normally used
 * for upstream content will be included in the statedir, otherwise
 * a subset that includes only directories used for 3rd-party repositories
 * will be created.
 */
int statedir_create_dirs(bool include_all);

/**
 * @brief Sets the path to the datadir.
 *
 * @param path The path of the datadir
 */
bool statedir_set_data_path(char *path);

/**
 * @brief Sets the path to the cachedir.
 *
 * @param path The path of the cachedir
 */
bool statedir_set_cache_path(char *path);

/**
 * @brief Sets the path to the duplicate of the cachedir.
 * This is mostly used for offline installs.
 *
 * @param path The path of the statedir duplicate
 */
bool statedir_dup_set_path(char *path);

#ifdef __cplusplus
}
#endif

#endif
