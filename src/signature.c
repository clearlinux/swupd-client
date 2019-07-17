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
#include <openssl/x509v3.h>

#include "config.h"
#include "lib/log.h"
#include "lib/macros.h"
#include "lib/strings.h"
#include "signature.h"

#ifdef SIGNATURES

static int validate_certificate(X509 *cert, const char *certificate_path, const char *crl);
static int verify_callback(int, X509_STORE_CTX *);
static X509 *get_cert_from_path(const char *certificate_path);

static X509_STORE *store = NULL;
static STACK_OF(X509) *x509_stack = NULL;

/* This function must be called before trying to sign any file.
 * It loads string for errors, and ciphers are auto-loaded by OpenSSL now.
 * If this function fails it may be because the certificate cannot
 * be validated.
 *
 * returns: true if can initialize and validate certificates, otherwise false */
bool signature_init(const char *certificate_path, const char *crl)
{
	int ret = -1;
	X509 *cert;

	if (!certificate_path) {
		error("Invalid swupd certificate - Empty");
		return false;
	}

	ERR_load_crypto_strings();
	ERR_load_PKCS7_strings();
	EVP_add_digest(EVP_sha256());

	cert = get_cert_from_path(certificate_path);
	if (!cert) {
		goto fail;
	}

	ret = validate_certificate(cert, certificate_path, crl);
	if (ret) {
		if (ret == X509_V_ERR_CERT_NOT_YET_VALID) {
			BIO *b;
			time_t currtime = 0;
			struct tm *timeinfo;
			char time_str[50];

			/* The system time wasn't sane, print out what it is and the cert validity range */
			time(&currtime);
			timeinfo = localtime(&currtime);

			strftime(time_str, sizeof(time_str), "%a %b %d %H:%M:%S %Y", timeinfo);
			warn("Current time is %s\n", time_str);
			info("Certificate validity is:\n");
			b = BIO_new_fp(stdout, BIO_NOCLOSE);
			if (b == NULL) {
				error("Failed to create BIO wrapping stream\n");
				goto fail;
			}
			/* The ASN1_TIME_print function does not include a newline... */
			if (!ASN1_TIME_print(b, X509_get_notBefore(cert))) {
				info("\n");
				error("Failed to get certificate begin date\n");
				goto fail;
			}
			info("\n");
			if (!ASN1_TIME_print(b, X509_get_notAfter(cert))) {
				info("\n");
				error("Failed to get certificate expiration date\n");
				goto fail;
			}
			info("\n");
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
	X509_free(cert);
	error("Failed to verify certificate: %s\n", X509_verify_cert_error_string(ret));
	return false;
}

/* Delete the memory used for string errors as well as memory allocated for
 * certificates and private keys. */
void signature_deinit(void)
{
	if (store) {
		X509_STORE_free(store);
		store = NULL;
	}
	if (x509_stack) {
		sk_X509_pop_free(x509_stack, X509_free);
		x509_stack = NULL;
	}
	ERR_free_strings();
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();
}

/* Verifies that the file and the signature exists, and does a signature check
 * afterwards. If any error is to be considered a verify failure, then
 * print_errors should be set to true.
 *
 * returns: true if able to validate the signature, false otherwise */
bool signature_verify(const char *file, const char *sig_file, bool print_errors)
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
	sig_fd = open(sig_file, O_RDONLY);
	if (sig_fd == -1) {
		string_or_die(&errorstr, "Failed open %s: %s\n", sig_file, strerror(errno));
		goto error;
	}
	if (fstat(sig_fd, &st) != 0) {
		string_or_die(&errorstr, "Failed to stat %s file\n", sig_file);
		goto error;
	}
	sig_len = st.st_size;
	sig = mmap(NULL, sig_len, PROT_READ, MAP_PRIVATE, sig_fd, 0);
	if (sig == MAP_FAILED) {
		string_or_die(&errorstr, "Failed to mmap %s signature\n", sig_file);
		goto error;
	}
	sig_BIO = BIO_new_mem_buf(sig, sig_len);
	if (!sig_BIO) {
		string_or_die(&errorstr, "Failed to read %s signature into BIO\n", sig_file);
		goto error;
	}

	/* the signature is in DER format, so d2i it into verification pkcs7 form */
	p7 = d2i_PKCS7_bio(sig_BIO, NULL);
	if (p7 == NULL) {
		string_or_die(&errorstr, "NULL PKCS7 File\n");
		goto error;
	}

	/* get the data to be verified */

	data_fd = open(file, O_RDONLY);
	if (data_fd == -1) {
		string_or_die(&errorstr, "Failed open %s\n", file);
		goto error;
	}
	if (fstat(data_fd, &st) != 0) {
		string_or_die(&errorstr, "Failed to stat %s\n", file);
		goto error;
	}
	data_len = st.st_size;
	data = mmap(NULL, data_len, PROT_READ, MAP_PRIVATE, data_fd, 0);
	if (data == MAP_FAILED) {
		string_or_die(&errorstr, "Failed to mmap %s\n", file);
		goto error;
	}
	data_BIO = BIO_new_mem_buf(data, data_len);
	if (!data_BIO) {
		string_or_die(&errorstr, "Failed to read %s into BIO\n", file);
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
	if (!result && print_errors) {
		error("Signature check error\n%s", errorstr);
		ERR_print_errors_fp(stderr);
	}

	free_string(&errorstr);

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

/* Returns a pointer to a parsed X509 certificate from file. */
static X509 *get_cert_from_path(const char *certificate_path)
{
	FILE *fp_pubkey = NULL;
	X509 *cert = NULL;

	fp_pubkey = fopen(certificate_path, "re");
	if (!fp_pubkey) {
		error("Failed fopen %s\n", certificate_path);
		goto error;
	}

	cert = PEM_read_X509(fp_pubkey, NULL, NULL, NULL);
	fclose(fp_pubkey);
	if (!cert) {
		error("Failed PEM_read_X509() for %s\n", certificate_path);
		goto error;
	}

	return cert;

error:
	ERR_print_errors_fp(stderr);
	return NULL;
}

/*
 * Check if a x509 extension is critical
 */
static int is_x509_ext_critical(X509 *cert, int nid)
{
	X509_EXTENSION *ex;
	ASN1_OBJECT *obj;
	int n, i;

	n = X509_get_ext_count(cert);
	for (i = 0; i < n; i++) {
		ex = X509_get_ext(cert, i);
		if (!ex) {
			continue;
		}
		obj = X509_EXTENSION_get_object(ex);
		if (!obj) {
			continue;
		}
		if (OBJ_obj2nid(obj) == nid && X509_EXTENSION_get_critical(ex)) {
			debug("Certificate nid %d is critical\n", nid);
			return 1;
		}
	}

	return 0;
}

/*
 * Check if the certificate is revoked using OCSP.
 * Current implementation aren't using OCSP to validate the certificate.
 * If the OCSP server is set and the cert Key Usage is set as critical,
 * stop swupd operation.
 *
 * TODO: Implement proper check of OCSP.
 */
static int validate_authority(X509 *cert)
{
	AUTHORITY_INFO_ACCESS *info = NULL;
	int n, i;

	// Check if Authority Information Access is critical
	if (!is_x509_ext_critical(cert, NID_info_access)) {
		return 0;
	}

	debug("Authority Information Access is critical. Checking certificate revocation method\n");

	// Check if the OCSP URI is set
	info = X509_get_ext_d2i(cert, NID_info_access, NULL, NULL);
	if (!info) {
		goto error;
	}

	n = sk_ACCESS_DESCRIPTION_num(info);
	for (i = 0; i < n; i++) {
		ACCESS_DESCRIPTION *ad = sk_ACCESS_DESCRIPTION_value(info, i);
		if (OBJ_obj2nid(ad->method) == NID_ad_OCSP) {
			error("OCSP uri found, but method not supported\n");
			goto error;
		}
	}

error:
	AUTHORITY_INFO_ACCESS_free(info);
	error("Supported Authority Information Access methods not found in the certificate\n");

	return -1;
}

/* This function makes sure the certificate is still valid by not having any
 * compromised certificates in the chain.
 * If there is no Certificate Revocation List (CRL) it may be that the private
 * keys have not been compromised or the CRL has not been generated by the
 * Certificate Authority (CA)
 *
 * returns: 0 if certificate is valid, X509 store error code otherwise */
static int validate_certificate(X509 *cert, const char *certificate_path, const char *crl)
{
	X509_LOOKUP *lookup = NULL;
	X509_STORE_CTX *verify_ctx = NULL;

	//TODO: Implement a chain verification when required

	/* create the cert store and set the verify callback */
	if (!(store = X509_STORE_new())) {
		error("Failed X509_STORE_new() for %s\n", certificate_path);
		goto error;
	}

	X509_STORE_set_verify_cb_func(store, verify_callback);

	if (X509_STORE_set_purpose(store, X509_PURPOSE_ANY) != 1) {
		error("Failed X509_STORE_set_purpose() for %s\n", certificate_path);
		goto error;
	}

	/* Add the certificates to be verified to the store */
	if (!(lookup = X509_STORE_add_lookup(store, X509_LOOKUP_file()))) {
		error("Failed X509_STORE_add_lookup() for %s\n", certificate_path);
		goto error;
	}

	/*  Load the our Root cert, which can be in either DER or PEM format */
	if (!X509_load_cert_file(lookup, certificate_path, X509_FILETYPE_PEM)) {
		error("Failed X509_load_cert_file() for %s\n", certificate_path);
		goto error;
	}

	if (crl) {
		if (!(lookup = X509_STORE_add_lookup(store, X509_LOOKUP_file())) ||
		    (X509_load_crl_file(lookup, crl, X509_FILETYPE_PEM) != 1)) {
			error("Failed X509 crl init for %s\n", certificate_path);
			goto error;
		}
		/* set the flags of the store so that CLRs are consulted */
		X509_STORE_set_flags(store, X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);
	}

	/* create a verification context and initialize it */
	if (!(verify_ctx = X509_STORE_CTX_new())) {
		error("Failed X509_STORE_CTX_new() for %s\n", certificate_path);
		goto error;
	}

	if (X509_STORE_CTX_init(verify_ctx, store, cert, NULL) != 1) {
		error("Failed X509_STORE_CTX_init() for %s\n", certificate_path);
		goto error;
	}
	/* Specify which cert to validate in the verify context.
	 * This is required because we may add multiple certs to the X509 store,
	 * but we want to validate a specific one out of the group/chain. */
	X509_STORE_CTX_set_cert(verify_ctx, cert);

	/* verify the certificate */
	if (X509_verify_cert(verify_ctx) != 1) {
		error("Failed X509_verify_cert() for %s\n", certificate_path);
		goto error;
	}

	X509_STORE_CTX_free(verify_ctx);

	if (validate_authority(cert) < 0) {
		error("Failed to validate certificate using 'Authority Information Access'\n");
		return -1;
	}

	/* Certificate verified correctly */
	return 0;

error:
	ERR_print_errors_fp(stderr);

	if (verify_ctx) {
		X509_STORE_CTX_free(verify_ctx);
	}

	return X509_STORE_CTX_get_error(verify_ctx);
}

static int verify_callback(int ok, X509_STORE_CTX *stor)
{
	if (!ok) {
		error("Certificate verification error - %s\n",
		      X509_verify_cert_error_string(X509_STORE_CTX_get_error(stor)));
	}
	return ok;
}

#else
bool signature_init(const char UNUSED_PARAM *certificate_path, const char UNUSED_PARAM *crl)
{
	return true;
}

void signature_deinit(void)
{
	return;
}

bool signature_verify(const char UNUSED_PARAM *file, const char UNUSED_PARAM *sig_file, bool UNUSED_PARAM print_errors)
{
	return true;
}
#endif
