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

#ifdef __cplusplus
}
#endif

#endif
