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

#define MAX_OPTIMIZED_BIT_POWER 1
#define SSE 0
#define AVX2 1 << 0
#define AVX512 1 << 1

#define SSE_0 0
#define SSE_1 1
#define SSE_2 2
#define SSE_3 3
#define AVX2_1 4
#define AVX2_3 5
#define AVX512_2 6
#define AVX512_3 7

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
	struct list *files;	/* struct file for files */
	struct list *manifests; /* struct file for possible manifests */

	// Helper data
	struct list *submanifests; /* struct manifest for subscribed manifests */
};

/**
 * @brief Get the optimization mask for the current system.
 *
 * @returns The bitwise OR of the system optimization level and optimization levels supported by the sytem.
 */
uint64_t get_opt_level_mask(void);

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

/**
 * @brief Create a list of manifests based on bundle and bundle dependencies.
 *
 * Explore bundle and its dependencies, optional and includes, recursively
 * adding all unique occurrences to @c manifests list. Filter is used to check
 * if a bundle should be explored or not.
 *
 * @param mom The Manifest of Manifest that @c bundle is in.
 * @param bundle The bundle name to explore
 * @param a pointer to a list that will be used to store unique bundle
 *        occurrences.
 * @param filter_fn a filter to apply to bundle names. If filter_fn() returns
 *        false the bundle will be skipped.
 * @return 0 on success or a negative error code on errors.
 *
 * @note If filter_fn(b) == false, where b is bundle or any of its dependencies,
 *       the bundle will not be added to the list and its dependencies aren't
 *       going to be explored.
 * @note if bundle is already in the list of manifests it won't be explored.
 *
 * @example
 *      struct list *manifest_list = NULL;
 *      err = recurse_dependencies(mom, "my_bundle", &manifest_list,
 *                                 is_installed_bundle);
 * This code will add to manifest_list my_bundle and all its direct and indirect
 * dependencies that are installed in the system. Note that if my_bundle isn't
 * installed the list will remain empty.
 */
int recurse_dependencies(struct manifest *mom, const char *bundle, struct list **manifests, filter_fn_t filter_fn);

#ifdef __cplusplus
}
#endif

#endif
