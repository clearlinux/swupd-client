#ifndef __SWUPD_COMP_FUNCTIONS__
#define __SWUPD_COMP_FUNCTIONS__

/**
 * @file
 * @brief Compare-like functions used as arguments of other swupd functions
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief compare file->filename with file->filename */
int cmp_file_filename(const void *a, const void *b);

/** @brief compare a pointer to file->filename with a pointer to file->filename */
int cmp_file_filename_ptr(const void *a, const void *b);

/** @brief compare file->hash with file->hash */
int cmp_file_hash(const void *a, const void *b);

/** @brief compare filerecord->filename with filerecord->filename */
int cmp_filerecord_filename(const void *a, const void *b);

/** @brief compare manifest->component with manifest->component */
int cmp_manifest_component(const void *a, const void *b);

/** @brief compare sub->component with sub->component */
int cmp_subscription_component(const void *a, const void *b);

/** @brief compare manifest->component with a string */
int cmp_manifest_component_string(const void *a, const void *b);

/** @brief compare file->filename with a string */
int cmp_file_filename_string(const void *a, const void *b);

/** @brief compare sub->component with a string */
int cmp_sub_component_string(const void *a, const void *b);

/** @brief compare a string with filerecord->filename */
int cmp_string_filerecord_filename(const void *a, const void *b);

/**
 * @brief compare file->filename with file->filename
 * @returns an inverse result
 */
int cmp_file_filename_reverse(const void *a, const void *b);

/** @brief compare file->hash, file->last_change with file->hash, file->last_change */
int cmp_file_hash_last_change(const void *a, const void *b);

/** @brief compare file->filename, file->is_deleted with file->filename, file->is_deleted */
int cmp_file_filename_is_deleted(const void *a, const void *b);

/** @brief compare repo->name with a string */
int cmp_repo_name_string(const void *a, const void *b);

/** @brief compare repo->url with a string */
int cmp_repo_url_string(const void *a, const void *b);

#ifdef __cplusplus
}
#endif
#endif
