#ifndef __MANIFEST__
#define __MANIFEST__

/**
 * @file
 * @brief Manifest parsing and processing.
 *
 * There are still several manifest functions on swupd.h that needs to me moved here.
 */

#include <stdbool.h>
#include <stdint.h>

#include "lib/list.h"

#ifdef __cplusplus
extern "C" {
#endif

struct manifest {
	// Header
	int manifest_version;
	int version;
	uint64_t filecount;
	uint64_t contentsize;
	struct list *includes; /* list of strings with manifest names */
	struct list *optional;
	char *component;

	// File list
	struct list *files;     /* struct file for files */
	struct list *manifests; /* struct file for possible manifests */

	// Helper data
	struct list *submanifests; /* struct manifest for subscribed manifests */
	unsigned int is_mix : 1;
};

/**
 * @brief Parse manifest located on disk under 'filename' and set it's name to 'component'.
 * @param header_only If set don't parse manifest files.
 *
 * @returns The parsed manifest.
 */
struct manifest *manifest_parse(const char *component, const char *filename, bool header_only);

/**
 * @brief Free manifest pointed by @c data.
 *
 * Just a wrapper on manifest_free() to help with casting.
 */
void manifest_free_data(void *data);

/**
 * @brief Free manifest pointed by 'manifest'
 */
void manifest_free(struct manifest *manifest);

/**
 * @brief Download if necessary all manifests from this mom and return the value
 *        in a list of manifests.
 * @param mom The MoM
 * @param manifest_list The list where downloaded manifests will be appended.
 *        If NULL, manifests are going to be freed.
 * @param filter_fn Filter to use for this operation. If filter_fn() is false,
 *        manifest will be ignored. If NULL, all manifets will be downloaded.
 */
int mom_get_manifests_list(struct manifest *mom, struct list **manifest_list, filter_fn_t filter_fn);

#ifdef __cplusplus
}
#endif

#endif
