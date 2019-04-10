#ifndef __ARCHIVES__
#define __ARCHIVES__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Extracts tar archive tarfile to outputdir. Refuses to extract any object
 * whose final location would be altered by a symlink on disk.
 */
int archives_extract_to(const char *tarfile, const char *outputdir);

#ifdef __cplusplus
}
#endif

#endif
