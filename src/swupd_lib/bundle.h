#ifndef __BUNDLE__
#define __BUNDLE__

/**
 * @file
 * @brief Common bundle related operations
 */

#include "manifest.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check if a bundle is installed in the system.
 * @param bundle_name The name of the bundle.
 *
 * @return True if a bundle is installed and false otherwise.
 */
bool is_installed_bundle(const char *bundle_name);

/**
 * @brief Check if a bundle is tracked in the system.
 * @param bundle_name The name of the bundle.
 *
 * @return True if a bundle is tracked and false otherwise.
 */
bool is_tracked_bundle(const char *bundle_name);

/**
 * @brief Create a list of bundles required by bundle_name.
 * TODO: Make this function more generic. Right now it's too specific for
 * bundle-list and bundle-remove use cases.
 * @param reqd_by A list where the bundles found will be stored
 * @param bundle_name The bundle we are looking for
 * @param mom The MoM
 * @param recursion A control variable used to sense what level of recursion the function is
 * @param exlusions A list of bundles that should not be considered when finding dependencies
 * @param msg The message to be printed if dependencies are found
 * @param include_optional A flag to indicate if we want to include optional dependencies in the list
 */
int required_by(struct list **reqd_by, const char *bundle_name, struct manifest *mom, int recursion, struct list *exclusions, char *msg, bool include_optional);

/**
 * @brief Creates a tracking file in the tracking directory within the
 * specified state directory.
 * If there are no tracked files in that directory (directory is empty
 * or does not exist), copy the tracking directory at
 * path_prefix/usr/share/clear/bundles to the tracking directory to
 * initiate the tracking files.
 * This function does not return an error code because weird state in this
 * directory must be handled gracefully whenever encountered.
 * @param bundle_name The name of the bundle.
 * @param state_dir The path to the state directory.
 */
void track_bundle_in_statedir(const char *bundle_name, const char *state_dir);

/**
 * @brief Similar to track_bundle_in_statedir() but uses the global state_dir.
 * @param bundle_name The name of the bundle.
 */
void track_bundle(const char *bundle_name);

/**
 * @brief Returns the list of tracked bundles.
 */
struct list *bundle_list_tracked(void);

/**
 * @brief Returns the list of installed bundles.
 */
struct list *bundle_list_installed(void);

/**
 * @brief Creates a list of orphaned bundles.
 * @param mom The MoM manifest
 * @bundles A list struct where the orphaned bundles will be added
 *
 * @return SWUPD_Ok if the list was created successfully, and an error code otherwise
 */
enum swupd_code bundle_list_orphans(struct manifest *mom, struct list **bundles);

#ifdef __cplusplus
}
#endif

#endif
