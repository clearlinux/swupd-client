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
 */
int required_by(struct list **reqd_by, const char *bundle_name, struct manifest *mom, int recursion, struct list *exclusions, char *msg);

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
 * @brief Validates that a tracking directory exists and that it is not
 * empty, if it is, it creates it and marks all installed bundles as tracked.
 * @param state_dir The path to the state directory.
 */
bool validate_tracking_dir(const char *state_dir);

#ifdef __cplusplus
}
#endif

#endif
