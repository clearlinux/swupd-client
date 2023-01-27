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

/** @brief Signature verification modifier flags */
enum signature_flags {
	/** @brief Perform default operation */
	SIGNATURE_DEFAULT = 0,

	/** @brief Verbose when printing errors */
	SIGNATURE_PRINT_ERRORS = 0x1,

	/** @brief Successful even when expire date has passed */
	SIGNATURE_IGNORE_EXPIRATION = 0x2,
};

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
 * @param flags        Flags to modify signature check
 *
 * @return true if the file is signed with certificate used signature_init()

 */
bool signature_verify(const char *file, const char *sig_file, enum signature_flags flags);

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
 * @param flags        Flags to modify signature check
 *
 * @return true if the file is signed with certificate used signature_init()
 */
bool signature_verify_data(const void *data, size_t data_len, const void *sig_data, size_t sig_data_len, enum signature_flags flags);

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
