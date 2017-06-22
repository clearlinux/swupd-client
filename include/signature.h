#ifndef SIGNATURE_H_
#define SIGNATURE_H_

#include <stdbool.h>

/*
 * Initialize this module.
 * @return true <=> no error
 */
bool initialize_signature(void);

/*
 * Terminate usage of this module, free resources.
 */
void terminate_signature(void);

/*
 * The given data file has already been downloaded from the given URL.
 * Download the corresponding signature file, and verify the data against the signature.
 * Delete the signature file if and only if the verification fails.
 * @param data_url - the URL from which the data came
 * @param data_filename - the file containing the data
 */
bool download_and_verify_signature(const char *data_url, const char *data_filename, int version, bool mix_exists);

#endif /* SIGNATURE_H_ */
