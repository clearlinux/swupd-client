#ifndef __SIGNATURE_H__
#define __SIGNATURE_H__

/**
 * @file
 * @brief Signature validation functions.
 */

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize this module.
 *
 * @param cert_path the path to the certificate to be used to check signatures.
 * @param crl       a certificate revocation list to be used to check if the
 *                  certificate is revoked. If null, the certificate is
 *                  considered valid.
 *
 * @return true if no error.
 */
bool signature_init(const char *certificate_path, const char *crl);

/**
 * Terminate usage of this module, free resources.
 */
void signature_deinit(void);

/**
 * Verify if file is signed.
 *
 * @param file         path to file to be verified.
 * @param sig_file     path to signature file.
 * @param print_errors if false, errors aren't printed.
 *
 * @return true if the file is signed with certificate used signature_init()

 */
bool signature_verify(const char *file, const char *sig_file, bool print_errors);

/**
 * Verify signature of a file in memory.
 *
 * Works like signature_verify(), but instead of pointing to file names
 * in disk, the content of the files should be in memory.
 *
 * @param data         Data to be verified
 * @param data_len     Length of data in bytes
 * @param sig          Data to be verified
 * @param sig_len      Length of data in bytes
 * @param print_errors if false, errors aren't printed.
 *
 * @return true if the file is signed with certificate used signature_init()
 */
bool signature_verify_data(const unsigned char *data, size_t data_len, const unsigned char *sig_data, size_t sig_data_len, bool print_errors);

/**
 * Print extra information about the certificate pointed by PATH.
 *
 * Currently printing Issuer, Subject and the dump of the certificate data.
 * Init is not required to run this function.
*
 * @param path the path to the certificate to be printed.
 */
void signature_print_info(const char *path);

#ifdef __cplusplus
}
#endif

#endif
