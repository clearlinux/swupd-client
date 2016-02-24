#ifndef __INCLUDE_GUARD_XATTRS_H
#define __INCLUDE_GUARD_XATTRS_H

#include <stdlib.h>

/*
 * Best effort to copy the extended attributes from one file to another.
 *
 * @param src_filename - The source file from which the extended attributes
 * will be read.
 * @param dst_filename - The destination file where the source file extended
 * attributes have to be copied.
 * @return - None.
 */
void xattrs_copy(const char *src_filename, const char *dst_filename);

/*
 * Attempt to pack into a data blob the extended attributes of the given file.
 * The data blob will be allocated and possibly filled with first the list
 * of all the extended attributes names, and then their values.
 * @param filename - The file from which the extended attributes will be
 * read and packed into the data blob.
 * @param blob - The data blob pointer into which the extended attributes will
 * be packed. (the data blob content has to be freed by the caller).
 * @param blob_len - The returned data blob length.
 * @return - None.
 */
void xattrs_get_blob(const char *filename, char **blob, size_t *blob_len);

/*
 * Compare the extended attributes from one file to another.
 *
 * @param filename1 - The first file used for the extended attributes
 * comparison.
 * @param filename2 - The second file used for the extended attributes
 * comparison.
 * @return - 0 if the extended attributes are identical, -1 if they are
 * different.
 */
int xattrs_compare(const char *filename1, const char *filename2);

#endif /* __INCLUDE_GUARD_XATTRS_H */
