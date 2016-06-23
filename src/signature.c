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
#include <sys/stat.h>
#include <unistd.h>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/crypto.h>

#include "config.h"
#include "signature.h"
#include "swupd.h"

#define BUFFER_SIZE   4096
#define CERTNAME "/usr/share/clear/update-ca/ClearLinuxRoot.pem"
#ifdef SIGNATURES
       bool verify_signatures = true;
#else
       bool verify_signatures = false;
#endif

static bool validate_signature(char *, FILE *, FILE *);
static bool validate_certificate(void);
static int verify_callback(int, X509_STORE_CTX *);
static bool get_pubkey();

static bool initialized = false;
static EVP_PKEY *pkey;
static X509 *cert;
static X509_STORE *store = NULL;
static char *chain;
static char *crl;

/* This function must be called before trying to sign any file.
 * It loads string for errors, and ciphers are auto-loaded by OpenSSL now.
 * If this function fails it may be because the certificate cannot
 * be validated.
 *
 * ca_cert_filename: is the swupd certificate that contains the public key
 * for signature verification.
 *
 * returns: true if can initialize and validate certificates, otherwise false */
bool initialize_signature(void)
{
	if (initialized) {
		return true;
	}

	ERR_load_crypto_strings();
	ERR_load_PKCS7_strings();
	if (!get_pubkey()) {
		goto fail;
	}
	if (!validate_certificate()) {
		goto fail;
	}

	initialized = true;
	return true;
fail:
	ERR_free_strings();
	EVP_cleanup();
	return false;
}

/* Delete the memory used for string errors as well as memory allocated for
 * certificates and private keys. */
void terminate_signature(void)
{
	if (initialized) {
		X509_free(cert);
		ERR_free_strings();
		EVP_PKEY_free(pkey);
		EVP_cleanup();
		initialized = false;
	}
}

/* Verifies that the file and the signature exists, and does a signature
 * check afterwards.
 *
 * returns: true if able to validate the signature, false otherwise */
bool verify_signature(const char *data_filename, const char *sig_filename)
{
	FILE *file = NULL;
	FILE *sig = NULL;
	bool result = false;

	if (!initialized) {
		return false;
	}

	sig = fopen(sig_filename, "rb");
	if (!sig) {
		fprintf(stderr, "Failed fopen %s\n", sig_filename);
		return false;
	}

	file = fopen(data_filename, "rb");
	if (!file) {
		fprintf(stderr, "Failed fopen %s\n", data_filename);
		fclose(sig);
		return false;
	}

	result = validate_signature(data_filename, file, sig);
	fclose(file);
	fclose(sig);

	return result;
}

/* Make sure the certificate exists and extract the public key from it.
 *
 * CERTNAME: certificate used to verify signature.
 *
 * returns: true if it can get the public key, false otherwise */
static bool get_pubkey(void)
{
	FILE *fp_pubkey = NULL;

	fp_pubkey = fopen(CERTNAME, "r");
	if (!fp_pubkey) {
		fprintf(stderr, "Failed fopen %s\n", CERTNAME);
		goto error;
	}

	cert = PEM_read_X509(fp_pubkey, NULL, NULL, NULL);
	if (!cert) {
		fclose(fp_pubkey);
		goto error;
	}

	pkey = X509_get_pubkey(cert);
	if (!pkey) {
		fclose(fp_pubkey);
		X509_free(cert);
		goto error;
	}
	return true;
error:
	ERR_print_errors_fp(stderr);
	return false;
}

/* This is the main part of the signature validation.
 * This function reads a file in chunks of 4096 bytes to create a hash from
 * the content, then verifies the hash using the publick key against the
 * signature.
 *
 * returns: true if signature was correct, false otherwise */
static bool validate_signature(char *data_filename, FILE *fp_data, FILE* fp_sig)
{
	unsigned char *data = NULL;
	unsigned char *signature = NULL;
	size_t sig_len;
	size_t data_size;
	size_t count;
	struct stat st;
	int fd;
	int ret = 0;
	EVP_MD_CTX *ctx = NULL;
	PKCS7 *p7;
	BIO *sigbio;
	BIO *databio;
	BIO *indata;
	STACK_OF(X509) *x509_stack;

	EVP_add_digest(EVP_sha256());

	/* Read in the .sig file */
	fd = fileno(fp_sig);
	if (fstat(fd, &st) != 0) {
		fprintf(stderr, "Failed to stat .sig file\n");
	}
	sig_len = st.st_size;
	signature = malloc(sig_len);
	count = fread(signature, 1, sig_len, fp_sig);
	if (count != sig_len) {
		fprintf(stderr, "Failed to get signature length! Read %lu/%lu bytes\n", count, sig_len);
		goto error;
	}

	/* Read in the corresponding data file for the .sig */
	fd = fileno(fp_data);
	if (fstat(fd, &st) != 0) {
		fprintf(stderr, "Failed to stat data file\n");
		goto error;
	}
	data_size = st.st_size;
	data = malloc(data_size);
	count = fread(data, 1, data_size, fp_data);
	if (count != data_size) {
		fprintf(stderr, "Failed to read full data file\n");
		goto error;
	}
	sigbio = BIO_new_mem_buf(signature, sig_len);
	p7 = d2i_PKCS7_bio(sigbio, NULL);
	if (p7 == NULL) {
		fprintf(stderr, "NULL PKCS7 File\n");
		goto error;
	}
	databio = BIO_new_mem_buf(data, data_size);
	indata = PKCS7_dataInit(p7, databio);

	x509_stack = sk_X509_new_null();
	sk_X509_push(x509_stack, cert);

	ret = PKCS7_verify(p7, x509_stack, store, indata, NULL, 0);
	if (ret ==1) {
		printf("VERIFY SUCCESS!\n");
		return true;
	}
error:
	if (signature) {
		free(signature);
	}
	if (data) {
		free(data);
	}
	ERR_print_errors_fp(stderr);
	fprintf(stderr, "Signature check failed\n");
	return false;
}

/* This function makes sure the certificate is still valid by not having any
 * compromised certificates in the chain.
 * If there is no Certificate Revocation List (CRL) it may be that the private
 * keys have not been compromised or the CRL has not been generated by the
 * Certificate Authority (CA)
 *
 * returns: true if certificate is valid, false otherwise */
static bool validate_certificate(void)
{
	FILE *fp = NULL;
	X509_LOOKUP *lookup = NULL;
	X509_STORE_CTX *verify_ctx = NULL;

	/* CRL and Chains are not required for the current setup, but we may
	 * implement them in the future */
	if (!crl) {
		printf("No certificate revocation list provided\n");
	}
	if (!chain) {
		printf("No certificate chain provided\n");
	}

	/* create the cert store and set the verify callback */
	if (!(store = X509_STORE_new())) {
		goto error;
	}

	X509_STORE_set_verify_cb_func(store, verify_callback);

	/* Add the certificates to be verified to the store */
	if (!(lookup = X509_STORE_add_lookup(store, X509_LOOKUP_file()))) {
		goto error;
	}

	/*  Load the our Root cert, which can be in either DER or PEM format */
	if (!X509_load_cert_file(lookup, CERTNAME, X509_FILETYPE_PEM)) {
		goto error;
	}

	if (crl) {
		if (!(lookup = X509_STORE_add_lookup(store, X509_LOOKUP_file())) ||
		   (X509_load_crl_file(lookup, crl, X509_FILETYPE_PEM) != 1)) {
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

	/* Specify which cert to validate in the verify context.
	 * This is required because we may add multiple certs to the X509 store,
	 * but we want to validate a specific one out of the group/chain. */
	X509_STORE_CTX_set_cert(verify_ctx, cert);

	/* verify the certificate */
	if (X509_verify_cert(verify_ctx) != 1) {
		goto error;
	}
	/* Certificate verified correctly */
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

/* Downloads the corresponding signature filename from the
 * swupd server.
 *
 * returns: true if signature was downloaded, false otherwise
 */
bool download_and_verify_signature(const char *data_url, const char *data_filename)
{
	char *sig_url;
	char *sig_filename;
	int ret;
	bool result;

	string_or_die(&sig_url, "%s.sig", data_url);

	string_or_die(&sig_filename, "%s.sig", data_filename);

	ret = swupd_curl_get_file(sig_url, sig_filename, NULL, NULL, false);
	if (ret) {
		result = false;
	} else {
		result = verify_signature(data_filename, sig_filename);
	}
	free(sig_filename);
	free(sig_url);
	return result;
}

/* Delete the signature file downloaded with download_and_verify_signature() */
void delete_signature(const char *data_filename)
{
	char *sig_filename;
	struct stat st;

	string_or_die(&sig_filename, "%s.sig", data_filename);
	if (stat(sig_filename, &st) == 0) {
		unlink(sig_filename);
	}
	free(sig_filename);
}

