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
 * @brief Create a list of bundles required by bundle_name.
 * TODO: Make this function more generic. Right now it's too specific for
 * bundle-list and bundle-remove use cases.
 */
int required_by(struct list **reqd_by, const char *bundle_name, struct manifest *mom, int recursion, struct list *exclusions, char *msg);

#ifdef __cplusplus
}
#endif

#endif
