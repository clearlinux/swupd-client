#ifndef __MANIFEST__
#define __MANIFEST__

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
	char *component;

	// File list
	struct list *files;     /* struct file for files */
	struct list *manifests; /* struct file for possible manifests */

	// Helper data
	struct list *submanifests; /* struct manifest for subscribed manifests */
	unsigned int is_mix : 1;
};

/*
 * Parse manifest located on disk under 'filename' and set it's name to 'component'.
 * If 'header_only', don't parse manifest files.
 *
 * Return the parsed manifest
 */
struct manifest *manifest_parse(const char *component, const char *filename, bool header_only);

/*
 * Free manifest pointed by 'data'.
 * Just a wrapper on free_manifest() to help with casting.
 */
void free_manifest_data(void *data);

/*
 * Free manifest pointed by 'manifest'
 */
void free_manifest(struct manifest *manifest);

#ifdef __cplusplus
}
#endif

#endif
