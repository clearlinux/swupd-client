#ifndef __ARCHIVES__
#define __ARCHIVES__

/**
 * @file
 * @brief Tarball extraction functions.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Extracts tar archive tarfile to outputdir.
 *
 * @param tarfile The tarball to be extracted.
 * @param outputdir The directory to extract to.
 *
 * Refuses to extract any object whose final location would be altered by a
 * symlink on disk.
 */
int archives_extract_to(const char *tarfile, const char *outputdir);

/**
 * @brief Check if this tarball is valid and if it contains only one file with the
 * specified name.
 *
 * Trailing '/' is ignored for directories on the tarball, so
 * don't include that on file.
 */
int archives_check_single_file_tarball(const char *tarfilename, const char *file);

#ifdef __cplusplus
}
#endif

#endif
