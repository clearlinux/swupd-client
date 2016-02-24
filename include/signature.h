#ifndef SIGNATURE_H_
#define SIGNATURE_H_

#include <stdbool.h>

/*
 * Initialize this module.
 * @param ca_cert_filename - the file containing the CA certificate
 * @return true <=> no error
 */
bool signature_initialize(const char *ca_cert_filename);

/*
 * Terminate usage of this module, free resources.
 */
void signature_terminate(void);

/*
 * Verify data file contents against a purported signature.
 * @param data_filename - the file containing the data
 * @param sig_filename - the file containing the signature
 * @return true <=> no error
 */
bool signature_verify(const char *data_filename, const char *sig_filename);

/*
 * The given data file has already been downloaded from the given URL.
 * Download the corresponding signature file, and verify the data against the signature.
 * Delete the signature file if and only if the verification fails.
 * @param data_url - the URL from which the data came
 * @param data_filename - the file containing the data
 */
bool signature_download_and_verify(const char *data_url, const char *data_filename);

/*
 * Delete the signature file corresponding to given data file.
 * @param data_filename - the file containing the data
 */
void signature_delete(const char *data_filename);

#endif /* SIGNATURE_H_ */
