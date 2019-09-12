#include <stdio.h>
#include <string.h>

#include "../../src/manifest.h"
#include "../../src/swupd.h"
#include "test_helper.h"

static void validate_file(struct list *files, const char *filename, int version, int hash, int is_dir, int is_file, int is_link, int is_deleted, int is_ghosted, int is_manifest, int is_config, int is_state, int is_boot, int is_experimental, int is_mix)
{
	struct list *list;
	char hash_str[SWUPD_HASH_LEN];
	snprintf(hash_str, SWUPD_HASH_LEN, "%064d", hash);

	for (list = files; list; list = list->next) {
		struct file *file = list->data;
		if (strcmp(filename, file->filename) == 0) {
			check(strcmp(file->hash, hash_str) == 0);
			check(file->is_dir == is_dir);
			check(file->is_file == is_file);
			check(file->is_link == is_link);
			check(file->is_deleted == is_deleted);
			check(file->is_ghosted == is_ghosted);
			check(file->is_manifest == is_manifest);
			check(file->is_config == is_config);
			check(file->is_state == is_state);
			check(file->is_boot == is_boot);
			check(file->is_experimental == is_experimental);
			check(file->is_mix== is_mix);

			check(file->last_change == version);

			return;
		}
	}

	printf("File not found (%s:%d)\n", __FILE__, __LINE__);
	exit(EXIT_FAILURE);
}

struct manifest *manifest_parse_test(const char *component, const char *dir, const char *filename, bool header_only)
{
	struct manifest *m;
	char *full_path;

	full_path = sys_path_join(dir, filename);
	m = manifest_parse(component, full_path, header_only);
	free(full_path);

	return m;
}

static void test_manifest_parse()
{
	struct manifest *manifest;
	char *dir;

	dir = sys_dirname(__FILE__);
	// parsing a missing file should fail
	manifest = manifest_parse_test("test", dir, "test/unit/missing", false);
	check(manifest == NULL);

	// Check if parser can parse the manifest header even if files are invalid
	manifest = manifest_parse_test("test", dir, "data/mom1", true);
	check(manifest != NULL);
	check(manifest->manifest_version == 1);
	check(manifest->version == 30);
	check(manifest->filecount == 123);
	check(manifest->contentsize == 789);
	free_manifest(manifest);

	
	// Manifest parser should fail on incorrect file list
	manifest = manifest_parse_test("test", dir, "data/mom1", false);
	check(manifest == NULL);

	// Check if parser can parse all different flags supported for the file list
	manifest = manifest_parse_test("test", dir, "data/mom2", false);
	check(manifest != NULL);
	check(list_len(manifest->files) == 14);
	check(list_len(manifest->manifests) == 1);

	validate_file(manifest->manifests, "m1", 10, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0);

	validate_file(manifest->files, "f1", 20, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	validate_file(manifest->files, "f2", 30, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	validate_file(manifest->files, "f3", 30, 3, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0);
	validate_file(manifest->files, "f4", 30, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	validate_file(manifest->files, "f5", 30, 5, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0);
	validate_file(manifest->files, "f6", 30, 6, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0);
	validate_file(manifest->files, "f7", 30, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0);
	validate_file(manifest->files, "f8", 30, 8, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0);
	validate_file(manifest->files, "f9", 30, 9, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0);
	validate_file(manifest->files, "f10", 30, 10, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0);
	validate_file(manifest->files, "f11", 30, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	validate_file(manifest->files, "f12", 30, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	validate_file(manifest->files, "f13", 30, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);
	validate_file(manifest->files, "f14", 30, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	free_manifest(manifest);

	// Missing MANIFEST keyword on header
	manifest = manifest_parse_test("test", dir, "data/mom_invalid1", false);
	check(manifest == NULL);

	// Invalid header format
	manifest = manifest_parse_test("test", dir, "data/mom_invalid2", false);
	check(manifest == NULL);

	free(dir);
}

int main() {
	test_manifest_parse();

	return 0;
}
