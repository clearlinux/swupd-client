/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2016 Intel Corporation.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 2 or later of the License.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Authors:
 *         Tom Keel <thomas.keel@intel.com>
 *
 */

#define _GNU_SOURCE
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>

#include "config.h"
#include "signature.h"
#include "swupd.h"

/*
 * Implementation flavors:
 *   FAKE ..... do nothing, always return success
 *   FORGIVE .. do everything, always return success
 *   REAL ..... do everything, return the real status
 */
#define IMPL_FAKE 0
#define IMPL_FORGIVE 1
#define IMPL_REAL 2

#warning "TODO pick signing scheme"
#if defined(SWUPD_LINUX_ROOTFS)
#define IMPL IMPL_REAL
#endif

#if IMPL != IMPL_FAKE


bool signing_enabled = false;

static bool validate_signature(FILE *, FILE *);
static bool validate_certificate(void);
static int verify_callback(int, X509_STORE_CTX *);
static bool get_pubkey(const char *);
static bool get_certificates(void);

static bool initialized = false;
static EVP_PKEY *pkey;
static X509 *cert;
static char *chain;
static char *crl;
static char *certificate;
static char *ca_dirname;


bool signature_initialize(const char *ca_cert_filename)
{
	if (initialized) {
		return true;
	}

	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();

	if (!get_certificates()) {
		return false;
	}

	if (!validate_certificate()) {
		ERR_free_strings(); // undoes ERR_load_crypto_strings
		EVP_cleanup();      // undoes OpenSSL_add_all_algorithms
		return false || (IMPL == IMPL_FORGIVE);
	}

	if (!get_pubkey(ca_cert_filename)) {
		return false || (IMPL == IMPL_FORGIVE);
	}

	initialized = true;
	return true;
}

void signature_terminate(void)
{
	if (initialized) {
		ERR_free_strings();	  // undoes ERR_load_crypto_strings
		EVP_cleanup();		  // undoes OpenSSL_add_all_algorithms
		EVP_PKEY_free(pkey);
		X509_free(cert);
		initialized = false;
	}
}

bool get_certificates(void)
{
	/* TODO - get paths and certificate names from Linux environment */


	return true;
error:
	return false;
}

bool signature_verify(const char *data_filename, const char *sig_filename)
{
    FILE *fp_data = NULL;
    FILE *fp_sig = NULL;
	bool result = false;

    if (!initialized) {
        return false || (IMPL == IMPL_FORGIVE);
    }

    fp_sig = fopen(sig_filename, "r");
    if (!fp_sig) {
        fprintf(stderr, "Failed fopen %s\n", sig_filename);
        goto error;
    }

    /* read data from file */
    fp_data = fopen(data_filename, "r");
    if (!fp_data) {
        fprintf(stderr, "Failed fopen %s\n", data_filename);
        goto error;
    }

    result = validate_signature(fp_data, fp_sig);

    return result;

error:
    fclose(fp_data);
    fclose(fp_sig);
    return result || (IMPL == IMPL_FORGIVE);
}

static bool get_pubkey(const char *ca_cert_filename)
{
    FILE *fp_pubkey = NULL;

    /* Read public key */
    fp_pubkey = fopen(ca_cert_filename, "r");
    if (!fp_pubkey) {
        fprintf(stderr, "Failed fopen %s\n", ca_cert_filename);
        goto error;
    }

    cert = PEM_read_X509(fp_pubkey, NULL, NULL, NULL);
    if (!cert) {
        goto error;
    }

    pkey = X509_get_pubkey(cert);
    if (!pkey) {
        goto error;
    }
    return true;

error:
	ERR_print_errors_fp(stderr);

	if (fp_pubkey) {
		fclose(fp_pubkey);
	}
	if (pkey) {
		EVP_PKEY_free(pkey);
	}
	if (cert) {
		X509_free(cert);
	}
	return false;
}

#define BUFFER_SIZE   4096
static bool validate_signature(FILE *fp_data, FILE *fp_sig)
{
    char buffer[BUFFER_SIZE];
    unsigned char sig_buffer[BUFFER_SIZE];
    EVP_MD_CTX md_ctx;

    size_t sig_len = fread(sig_buffer, 1, BUFFER_SIZE, fp_sig);
    printf("size of signature: %lu\n", sig_len);


    /* get size of file */
    fseek(fp_data, 0, SEEK_END);
    size_t data_size = ftell(fp_data);
    fseek(fp_data, 0, SEEK_SET);

    if (!EVP_VerifyInit(&md_ctx, EVP_sha256())) {
        goto error;
    }

    /* read all bytes from file to calculate digest using sha256 and then sign it */
    size_t len = 0;
    size_t bytes_left = data_size;
    while (bytes_left > 0) {
        const size_t count = (bytes_left > BUFFER_SIZE ? BUFFER_SIZE : bytes_left);
        len = fread(buffer, 1, count, fp_data);
        if (len != count) {
            fprintf(stderr, "Failed len!= count\n");
            goto error;
        }

        if (!EVP_VerifyUpdate(&md_ctx, buffer, len)) {
            goto error;
        }
        bytes_left -= len;
    }

    /* Do the signature */
    if (!EVP_VerifyFinal(&md_ctx, sig_buffer, sig_len, pkey)) {
        goto error;
    } else {
        printf("Correct signature\n");
    }

    return true;

error:
	ERR_print_errors_fp(stderr);
	return false;
}

static bool validate_certificate(void)
{

    X509_STORE *store = NULL;
    FILE *fp = NULL;
    X509_LOOKUP *lookup = NULL;
    X509_STORE_CTX *verify_ctx = NULL;

    if (!crl) {
    	printf("No certificate revocation list provided\n");
    }
    if (!chain) {
        fprintf(stderr, "No certificate chain provided\n");
        goto error;
    }
    if (!certificate) {
        fprintf(stderr, "No certificate provided\n");
        goto error;
    }

    if (!(fp = fopen(certificate, "r"))) {
        fprintf(stderr, "Cannot open certificate\n");
        goto error;
    }
    if (!(cert = PEM_read_X509(fp, NULL, NULL, NULL))) {
        fprintf(stderr, "Cannot read x509 from certificate\n");
        goto error;
    }

    /* create the cert store and set the verify callback */
    if (!(store = X509_STORE_new())) {
        goto error;
    }

    X509_STORE_set_verify_cb_func(store, verify_callback);

    /* load the CA certificates and CRLs */
	if (X509_STORE_load_locations(store, chain, ca_dirname) != 1) {
        goto error;
	}

	if (X509_STORE_set_default_paths(store) != 1) {
        goto error;
	}

    if (crl) {
	    if (!(lookup = X509_STORE_add_lookup(store, X509_LOOKUP_file()))) {
            goto error;
        }

	    if (X509_load_crl_file(lookup, crl, X509_FILETYPE_PEM) != 1) {
            goto error;
	    }

        /* set the flags of the store so that CLRs are consulted */
	    X509_STORE_set_flags(store, X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);
    }

    /* create a verification context and initialize it */
    if (!(verify_ctx = X509_STORE_CTX_new())) {
        goto error;
    }

    if (X509_STORE_CTX_init(verify_ctx, store, cert, NULL) != 1) {
        goto error;
    }

    /* verify the certificate */
    if (X509_verify_cert(verify_ctx) != 1) {
        goto error;
    } else {
        printf("Certificate verified correctly!\n");
    	goto error;
    }

	return true;

error:
    ERR_print_errors_fp(stderr);

    if (fp) {
        fclose(fp);
    }
    if (store) {
    	X509_STORE_free(store);
    }
    if (lookup) {
    	X509_LOOKUP_free(lookup);
    }
	return false;
}

int verify_callback(int ok, X509_STORE_CTX *stor)
{
    if (!ok) {
        fprintf(stderr, "Error: %s\n",
            X509_verify_cert_error_string(stor->error));
    }
    return ok;
}

bool signature_download_and_verify(const char *data_url, const char *data_filename)
{
	char *sig_url;
	char *sig_filename;
	int ret;
	bool result;

	string_or_die(&sig_url, "%s.signed", data_url);

	string_or_die(&sig_filename, "%s.signed", data_filename);

	ret = swupd_curl_get_file(sig_url, sig_filename, NULL, NULL, false);
	if (ret) {
		result = false;
	} else {
		result = signature_verify(data_filename, sig_filename);
	}
	if (!result) {
		unlink(sig_filename);
	}
	free(sig_filename);
	free(sig_url);
	return result || (IMPL == IMPL_FORGIVE);
}

void signature_delete(const char *data_filename)
{
	char *sig_filename;

	string_or_die(&sig_filename, "%s.signed", data_filename);

	unlink(sig_filename);

	free(sig_filename);
}

#else // IMPL == IMPL_FAKE

bool signature_initialize(const char UNUSED_PARAM *ca_cert_filename)
{
	return true;
}

void signature_terminate(void)
{
}

bool signature_verify(const char UNUSED_PARAM *data_filename, const char UNUSED_PARAM *sig_filename)
{
	return true;
}

bool signature_download_and_verify(const char UNUSED_PARAM *data_url, const char UNUSED_PARAM *data_filename)
{
	return true;
}

void signature_delete(const char UNUSED_PARAM *data_filename)
{
}

#endif // IMPL == IMPL_FAKE
