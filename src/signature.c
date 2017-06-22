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
 *         Tudor Marcu <tudor.marcu@intel.com>
 *         Tim Pepper <timothy.c.pepper@linux.intel.com>
 *
 */

#define _GNU_SOURCE
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <openssl/bio.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/pem.h>

#include "config.h"
#include "signature.h"
#include "swupd.h"

#ifdef SIGNATURES

static char *CERTNAME;

static int validate_certificate(void);
static int verify_callback(int, X509_STORE_CTX *);
static bool get_pubkey();

FILE *fp_pubkey = NULL;
EVP_PKEY *pkey = NULL;
X509 *cert = NULL;
X509_STORE *store = NULL;
STACK_OF(X509) *x509_stack = NULL;
//TODO: static char *chain = NULL;
static char *crl = NULL;

/* This function must be called before trying to sign any file.
 * It loads string for errors, and ciphers are auto-loaded by OpenSSL now.
 * If this function fails it may be because the certificate cannot
 * be validated.
 *
 * returns: true if can initialize and validate certificates, otherwise false */
bool initialize_signature(void)
{
	int ret = -1;

	string_or_die(&CERTNAME, "%s", cert_path);

	ERR_load_crypto_strings();
	ERR_load_PKCS7_strings();
	EVP_add_digest(EVP_sha256());

	if (!get_pubkey()) {
		goto fail;
	}

	ret = validate_certificate();
	if (ret) {
		if (ret == X509_V_ERR_CERT_NOT_YET_VALID) {
			BIO *b;
			time_t currtime = 0;
			struct tm *timeinfo;

			/* The system time wasn't sane, print out what it is and the cert validity range */
			time(&currtime);
			timeinfo = localtime(&currtime);
			fprintf(stderr, "Warning: Current time is %s\n", asctime(timeinfo));
			fprintf(stderr, "Certificate validity is:\n");
			b = BIO_new_fp(stdout, BIO_NOCLOSE);
			if (b == NULL) {
				fprintf(stderr, "Failed to create BIO wrapping stream\n");
				goto fail;
			}
			/* The ASN1_TIME_print function does not include a newline... */
			if (!ASN1_TIME_print(b, X509_get_notBefore(cert))) {
				fprintf(stderr, "\nFailed to get certificate begin date\n");
				goto fail;
			}
			fprintf(stderr, "\n");
			if (!ASN1_TIME_print(b, X509_get_notAfter(cert))) {
				fprintf(stderr, "\nFailed to get certificate expiration date\n");
				goto fail;
			}
			fprintf(stderr, "\n");
			BIO_free(b);
		}
		goto fail;
	}

	/* Push our trust cert(s) to the stack, which is a set of certificates
	 * in which to search for the signer's cert. */
	x509_stack = sk_X509_new_null();
	if (!x509_stack) {
		goto fail;
	}
	sk_X509_push(x509_stack, cert);

	return true;
fail:
	fprintf(stderr, "Failed to verify certificate: %s\n", X509_verify_cert_error_string(ret));
	return false;
}

/* Delete the memory used for string errors as well as memory allocated for
 * certificates and private keys. */
void terminate_signature(void)
{
	//TODO: once implemented, must free chain
	//TODO: once implemented, must free crl
	if (store) {
		X509_STORE_free(store);
		store = NULL;
	}
	if (x509_stack) {
		sk_X509_pop_free(x509_stack, X509_free);
		x509_stack = NULL;
	}
	ERR_free_strings();
	if (pkey) {
		EVP_PKEY_free(pkey);
		pkey = NULL;
	}
	EVP_cleanup();
	if (fp_pubkey) {
		fclose(fp_pubkey);
	}
	if (cert) {
		cert = NULL;
	}
	ERR_remove_thread_state(NULL);
	CRYPTO_cleanup_all_ex_data();
}

/* Verifies that the file and the signature exists, and does a signature check
 * afterwards. If any error is to be considered a verify failure, then
 * ERRORS_FATAL should be set to true.
 *
 * returns: true if able to validate the signature, false otherwise */
static bool verify_signature(const char *data_filename, const char *sig_filename, bool errors_fatal)
{
	int ret;
	bool result = false;
	struct stat st;
	char *errorstr = NULL;

	int data_fd = -1;
	size_t data_len;
	unsigned char *data = NULL;
	BIO *data_BIO = NULL;

	int sig_fd = -1;
	size_t sig_len;
	unsigned char *sig = NULL;
	BIO *sig_BIO = NULL;

	PKCS7 *p7 = NULL;
	BIO *verify_BIO = NULL;

	/* get the signature */
	sig_fd = open(sig_filename, O_RDONLY);
	if (sig_fd == -1) {
		string_or_die(&errorstr, "Failed open %s: %s\n", sig_filename, strerror(errno));
		goto error;
	}
	if (fstat(sig_fd, &st) != 0) {
		string_or_die(&errorstr, "Failed to stat %s file\n", sig_filename);
		goto error;
	}
	sig_len = st.st_size;
	sig = mmap(NULL, sig_len, PROT_READ, MAP_PRIVATE, sig_fd, 0);
	if (sig == MAP_FAILED) {
		string_or_die(&errorstr, "Failed to mmap %s signature\n", sig_filename);
		goto error;
	}
	sig_BIO = BIO_new_mem_buf(sig, sig_len);
	if (!sig_BIO) {
		string_or_die(&errorstr, "Failed to read %s signature into BIO\n", sig_filename);
		goto error;
	}

	/* the signature is in DER format, so d2i it into verification pkcs7 form */
	p7 = d2i_PKCS7_bio(sig_BIO, NULL);
	if (p7 == NULL) {
		string_or_die(&errorstr, "NULL PKCS7 File\n");
		goto error;
	}

	/* get the data to be verified */
	data_fd = open(data_filename, O_RDONLY);
	if (data_fd == -1) {
		string_or_die(&errorstr, "Failed open %s\n", data_filename);
		goto error;
	}
	if (fstat(data_fd, &st) != 0) {
		string_or_die(&errorstr, "Failed to stat %s\n", data_filename);
		goto error;
	}
	data_len = st.st_size;
	data = mmap(NULL, data_len, PROT_READ, MAP_PRIVATE, data_fd, 0);
	if (data == MAP_FAILED) {
		string_or_die(&errorstr, "Failed to mmap %s\n", data_filename);
		goto error;
	}
	data_BIO = BIO_new_mem_buf(data, data_len);
	if (!data_BIO) {
		string_or_die(&errorstr, "Failed to read %s into BIO\n", data_filename);
		goto error;
	}

	/* munge the signature and data into a verifiable format */
	verify_BIO = PKCS7_dataInit(p7, data_BIO);
	if (!verify_BIO) {
		string_or_die(&errorstr, "Failed PKCS7_dataInit()\n");
		goto error;
	}

	/* Verify the signature, outdata can be NULL because we don't use it */
	ret = PKCS7_verify(p7, x509_stack, store, verify_BIO, NULL, 0);
	if (ret == 1) {
		result = true;
	} else {
		string_or_die(&errorstr, "Signature check failed!\n");
	}

error:
	if (!result && errors_fatal) {
		fputs(errorstr, stderr);
		ERR_print_errors_fp(stderr);
	}

	free(errorstr);

	if (sig) {
		munmap(sig, sig_len);
	}
	if (sig_fd >= 0) {
		close(sig_fd);
	}
	if (data) {
		munmap(data, data_len);
	}
	if (data_fd >= 0) {
		close(data_fd);
	}
	if (sig_BIO) {
		BIO_free(sig_BIO);
	}
	if (data_BIO) {
		BIO_free(data_BIO);
	}
	if (verify_BIO) {
		BIO_free(verify_BIO);
	}
	if (p7) {
		PKCS7_free(p7);
	}

	return result;
}

/* Make sure the certificate exists and extract the public key from it.
 *
 * returns: true if it can get the public key, false otherwise */
static bool get_pubkey(void)
{
	fp_pubkey = fopen(CERTNAME, "re");
	if (!fp_pubkey) {
		fprintf(stderr, "Failed fopen %s\n", CERTNAME);
		goto error;
	}

	cert = PEM_read_X509(fp_pubkey, NULL, NULL, NULL);
	if (!cert) {
		fprintf(stderr, "Failed PEM_read_X509() for %s\n", CERTNAME);
		goto error;
	}

	pkey = X509_get_pubkey(cert);
	if (!pkey) {
		fprintf(stderr, "Failed X509_get_pubkey() for %s\n", CERTNAME);
		X509_free(cert);
		goto error;
	}
	return true;
error:
	ERR_print_errors_fp(stderr);
	return false;
}

/* This function makes sure the certificate is still valid by not having any
 * compromised certificates in the chain.
 * If there is no Certificate Revocation List (CRL) it may be that the private
 * keys have not been compromised or the CRL has not been generated by the
 * Certificate Authority (CA)
 *
 * returns: 0 if certificate is valid, X509 store error code otherwise */
static int validate_certificate(void)
{
	X509_LOOKUP *lookup = NULL;
	X509_STORE_CTX *verify_ctx = NULL;

	/* TODO: CRL and Chains are not required for the current setup, but we may
	 * implement them in the future 
	if (!crl) {
		fprintf(stderr, "No certificate revocation list provided\n");
	}
	if (!chain) {
		fprintf(stderr, "No certificate chain provided\n");
	}
	*/

	/* create the cert store and set the verify callback */
	if (!(store = X509_STORE_new())) {
		fprintf(stderr, "Failed X509_STORE_new() for %s\n", CERTNAME);
		goto error;
	}

	X509_STORE_set_verify_cb_func(store, verify_callback);

	/* Add the certificates to be verified to the store */
	if (!(lookup = X509_STORE_add_lookup(store, X509_LOOKUP_file()))) {
		fprintf(stderr, "Failed X509_STORE_add_lookup() for %s\n", CERTNAME);
		goto error;
	}

	/*  Load the our Root cert, which can be in either DER or PEM format */
	if (!X509_load_cert_file(lookup, CERTNAME, X509_FILETYPE_PEM)) {
		fprintf(stderr, "Failed X509_load_cert_file() for %s\n", CERTNAME);
		goto error;
	}

	if (crl) {
		if (!(lookup = X509_STORE_add_lookup(store, X509_LOOKUP_file())) ||
		    (X509_load_crl_file(lookup, crl, X509_FILETYPE_PEM) != 1)) {
			fprintf(stderr, "Failed X509 crl init for %s\n", CERTNAME);
			goto error;
		}
		/* set the flags of the store so that CLRs are consulted */
		X509_STORE_set_flags(store, X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);
	}

	/* create a verification context and initialize it */
	if (!(verify_ctx = X509_STORE_CTX_new())) {
		fprintf(stderr, "Failed X509_STORE_CTX_new() for %s\n", CERTNAME);
		goto error;
	}

	if (X509_STORE_CTX_init(verify_ctx, store, cert, NULL) != 1) {
		fprintf(stderr, "Failed X509_STORE_CTX_init() for %s\n", CERTNAME);
		goto error;
	}

	/* Specify which cert to validate in the verify context.
	 * This is required because we may add multiple certs to the X509 store,
	 * but we want to validate a specific one out of the group/chain. */
	X509_STORE_CTX_set_cert(verify_ctx, cert);

	/* verify the certificate */
	if (X509_verify_cert(verify_ctx) != 1) {
		fprintf(stderr, "Failed X509_verify_cert() for %s\n", CERTNAME);
		goto error;
	}

	X509_STORE_CTX_free(verify_ctx);

	/* Certificate verified correctly */
	return 0;

error:
	ERR_print_errors_fp(stderr);

	if (verify_ctx) {
		X509_STORE_CTX_free(verify_ctx);
	}

	return verify_ctx->error;
}

int verify_callback(int ok, X509_STORE_CTX *stor)
{
	if (!ok) {
		fprintf(stderr, "Certificate verification error: %s\n",
			X509_verify_cert_error_string(stor->error));
	}
	return ok;
}

/* Verifies signature for the local file DATA_FILENAME first, and on failure
 * downloads the signature based on DATA_URL and tries to verify again.
 *
 * returns: true if signature verification succeeded, false if verification
 * failed, or the signature download failed
 */
bool download_and_verify_signature(const char *data_url, const char *data_filename, int version, bool mix_exists)
{
	char *local;
	char *sig_url;
	char *sig_filename;
	int ret;
	bool result;

	if (!sigcheck) {
		return false;
	}

	string_or_die(&sig_url, "%s.sig", data_url);
	string_or_die(&sig_filename, "%s.sig", data_filename);
	string_or_die(&local, "%s/%i/Manifest.MoM.sig", MIX_STATE_DIR, version);

	// Try verifying a local copy of the signature first
	result = verify_signature(data_filename, sig_filename, false);
	if (result) {
		goto out;
	}

	// Else, download a fresh signature, and verify
	if (mix_exists) {
		ret = link(local, sig_filename);
	} else {
		ret = swupd_curl_get_file(sig_url, sig_filename, NULL, NULL, false);
	}

	if (ret == 0) {
		result = verify_signature(data_filename, sig_filename, true);
	} else {
		// download failed
		result = false;
	}
out:
	free(sig_filename);
	free(sig_url);
	return result;
}

#else
bool initialize_signature(void)
{
	return true;
}

void terminate_signature(void)
{
	return;
}

bool download_and_verify_signature(const char UNUSED_PARAM *data_url, const char UNUSED_PARAM *data_filename)
{
	return true;
}
#endif
