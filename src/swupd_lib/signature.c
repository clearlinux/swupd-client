/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2025 Intel Corporation.
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

#include "lib/log.h"
#include "lib/macros.h"
#include "signature.h"
#include "swupd.h"

#ifdef SIGNATURES

static int validate_certificate(X509 *cert, const char *certificate_path, const char *crl);
static X509 *get_cert_from_path(const char *certificate_path);

static X509_STORE *store = NULL;
static STACK_OF(X509) *x509_stack = NULL;
static char *orig_cert = NULL;
static char *alt_cert = NULL;

static int verify_callback_ignore_expiration(int ok, X509_STORE_CTX *local_store)
{
	int error;

	if (!ok) {
		error = X509_STORE_CTX_get_error(local_store);
		debug("Certificate verification error - %s\n",
		      X509_verify_cert_error_string(error));
		if (error == X509_V_ERR_CERT_HAS_EXPIRED) {
			debug("Sinature is expired, but operation will proceed\n");
			return 1;
		}
	}

	return ok;
}

static int verify_callback(int ok, X509_STORE_CTX *local_store)
{
	int error;

	if (!ok) {
		error = X509_STORE_CTX_get_error(local_store);
		debug("Certificate verification error - %s\n",
		      X509_verify_cert_error_string(error));
	}

	return ok;
}

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
		error("Invalid swupd certificate - Empty\n");
		return false;
	}

	ERR_load_crypto_strings();
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
				debug("Failed to create BIO wrapping stream\n");
				goto fail;
			}
			/* The ASN1_TIME_print function does not include a newline... */
			if (!ASN1_TIME_print(b, X509_get_notBefore(cert))) {
				info("\n");
				debug("Failed to get certificate begin date\n");
				goto fail;
			}
			info("\n");
			if (!ASN1_TIME_print(b, X509_get_notAfter(cert))) {
				info("\n");
				debug("Failed to get certificate expiration date\n");
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

static bool signature_verify_data_internal(const void *data, size_t data_len, const void *sig_data, size_t sig_data_len, enum signature_flags flags)
{
	bool result = false;
	int ret;

	BIO *sig_BIO = NULL;
	BIO *data_BIO = NULL;
	BIO *verify_BIO = NULL;
	char *errorstr = NULL;
	PKCS7 *p7 = NULL;
	int sig_data_len_int, data_len_int;

	if (ulong_to_int_err(sig_data_len, &sig_data_len_int) != 0) {
		error("Data to big to be a signature file (size = %ld)\n", sig_data_len);
		goto error;
	}

	if (ulong_to_int_err(data_len, &data_len_int) != 0) {
		error("Data to big to verify signature (size = %ld)\n", data_len);
		goto error;
	}

	sig_BIO = BIO_new_mem_buf(sig_data, sig_data_len_int);
	if (!sig_BIO) {
		string_or_die(&errorstr, "Unable to load signature data into BIO");
		goto error;
	}

	/* the signature is in DER format, so d2i it into verification pkcs7 form */
	p7 = d2i_PKCS7_bio(sig_BIO, NULL);
	if (p7 == NULL) {
		string_or_die(&errorstr, "NULL PKCS7 File");
		goto error;
	}

	data_BIO = BIO_new_mem_buf(data, data_len_int);
	if (!data_BIO) {
		string_or_die(&errorstr, "Unable to load data into BIO");
		goto error;
	}

	/* munge the signature and data into a verifiable format */
	verify_BIO = PKCS7_dataInit(p7, data_BIO);
	if (!verify_BIO) {
		string_or_die(&errorstr, "Failed PKCS7_dataInit()");
		goto error;
	}

	/* Always set correct verify callback */
	if (flags & SIGNATURE_IGNORE_EXPIRATION) {
		X509_STORE_set_verify_cb_func(store, verify_callback_ignore_expiration);
	} else {
		X509_STORE_set_verify_cb_func(store, verify_callback);
	}

	/* Verify the signature, outdata can be NULL because we don't use it */
	ret = PKCS7_verify(p7, x509_stack, store, verify_BIO, NULL, 0);
	if (ret == 1) {
		result = true;
	} else {
		string_or_die(&errorstr, "Signature check failed");
	}

	/* Restore original callback */
	X509_STORE_set_verify_cb_func(store, verify_callback);

error:

	if (!result && flags & SIGNATURE_PRINT_ERRORS) {
		if (errorstr) {
			debug("%s\n", errorstr);
		}
	}

	FREE(errorstr);

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

static bool swap_certs(void)
{
	bool ret = true;

	signature_deinit();
	if (str_cmp(globals.cert_path, orig_cert) == 0) {
		set_cert_path(alt_cert);
	} else {
		set_cert_path(orig_cert);
	}
	if (!signature_init(globals.cert_path, NULL)) {
		ret = false;
	}

	return ret;
}

static bool use_alt_cert(void)
{
	bool ret = true;

	if (!globals.cert_path) {
		return false;
	}

	/* This function can be called multiple times and
	 * is intended to be able to handle swapping back
	 * and forth between main and alt cert files. */
	if (!orig_cert && !alt_cert) {
		orig_cert = strdup_or_die(globals.cert_path);
		string_or_die(&alt_cert, "%s.alt", globals.cert_path);
		if (sys_file_exists(alt_cert)) {
			warn("Default cert failed, attempting to use alternative: %s\n", alt_cert);
			ret = swap_certs();
		} else {
			FREE(alt_cert);
			alt_cert = NULL;
			ret = false;
		}
	} else if (!alt_cert) {
		ret = false;
	} else {
		ret = swap_certs();
	}

	return ret;
}

bool signature_verify_data(const void *data, size_t data_len, const void *sig_data, size_t sig_data_len, enum signature_flags flags)
{
	bool result = signature_verify_data_internal(data, data_len, sig_data, sig_data_len, flags);
	if (!result && use_alt_cert()) {
		result = signature_verify_data_internal(data, data_len, sig_data, sig_data_len, flags);
	}

	return result;
}

bool signature_verify(const char *file, const char *sig_file, enum signature_flags flags)
{
	bool result = false;
	size_t data_len;
	void *data = NULL;
	size_t sig_len;
	void *sig = NULL;

	/* get the signature */
	sig = sys_mmap_file(sig_file, &sig_len);
	if (!sig) {
		goto error;
	}

	/* get the data to be verified */
	data = sys_mmap_file(file, &data_len);
	if (!data) {
		goto error;
	}

	result = signature_verify_data(data, data_len, sig, sig_len, flags);

error:
	if (!result && (flags & SIGNATURE_PRINT_ERRORS)) {
		warn("Signature check failed\n");
	}
	sys_mmap_free(sig, sig_len);
	sys_mmap_free(data, data_len);

	return result;
}

/* Returns a pointer to a parsed X509 certificate from file. */
static X509 *get_cert_from_path(const char *certificate_path)
{
	FILE *fp_pubkey = NULL;
	X509 *cert = NULL;

	fp_pubkey = fopen(certificate_path, "re");
	if (!fp_pubkey) {
		debug("Failed fopen %s (%i - %s)\n", certificate_path, errno, strerror(errno));
		goto error;
	}

	cert = PEM_read_X509(fp_pubkey, NULL, NULL, NULL);
	fclose(fp_pubkey);
	if (!cert) {
		debug("Failed PEM_read_X509() for %s\n", certificate_path);
		goto error;
	}

	return cert;

error:
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
			debug("OCSP uri found, but method not supported\n");
			goto error;
		}
	}

error:
	AUTHORITY_INFO_ACCESS_free(info);
	debug("Supported Authority Information Access methods not found in the certificate\n");

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

	// TODO: Implement a chain verification when required

	/* create the cert store and set the verify callback */
	if (!(store = X509_STORE_new())) {
		debug("Failed X509_STORE_new() for %s\n", certificate_path);
		goto error;
	}

	X509_STORE_set_verify_cb_func(store, verify_callback);

	if (X509_STORE_set_purpose(store, X509_PURPOSE_ANY) != 1) {
		debug("Failed X509_STORE_set_purpose() for %s\n", certificate_path);
		goto error;
	}

	/* Add the certificates to be verified to the store */
	if (!(lookup = X509_STORE_add_lookup(store, X509_LOOKUP_file()))) {
		debug("Failed X509_STORE_add_lookup() for %s\n", certificate_path);
		goto error;
	}

	/*  Load the our Root cert, which can be in either DER or PEM format */
	if (!X509_load_cert_file(lookup, certificate_path, X509_FILETYPE_PEM)) {
		debug("Failed X509_load_cert_file() for %s\n", certificate_path);
		goto error;
	}

	if (crl) {
		if (!(lookup = X509_STORE_add_lookup(store, X509_LOOKUP_file())) ||
		    (X509_load_crl_file(lookup, crl, X509_FILETYPE_PEM) != 1)) {
			debug("Failed X509 crl init for %s\n", certificate_path);
			goto error;
		}
		/* set the flags of the store so that CLRs are consulted */
		X509_STORE_set_flags(store, X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);
	}

	/* create a verification context and initialize it */
	if (!(verify_ctx = X509_STORE_CTX_new())) {
		debug("Failed X509_STORE_CTX_new() for %s\n", certificate_path);
		goto error;
	}

	if (X509_STORE_CTX_init(verify_ctx, store, cert, NULL) != 1) {
		debug("Failed X509_STORE_CTX_init() for %s\n", certificate_path);
		goto error;
	}
	/* Specify which cert to validate in the verify context.
	 * This is required because we may add multiple certs to the X509 store,
	 * but we want to validate a specific one out of the group/chain. */
	X509_STORE_CTX_set_cert(verify_ctx, cert);

	/* verify the certificate */
	if (X509_verify_cert(verify_ctx) != 1) {
		debug("Failed X509_verify_cert() for %s\n", certificate_path);
		goto error;
	}

	X509_STORE_CTX_free(verify_ctx);

	if (validate_authority(cert) < 0) {
		debug("Failed to validate certificate using 'Authority Information Access'\n");
		return -1;
	}

	/* Certificate verified correctly */
	return 0;

error:
	if (verify_ctx) {
		X509_STORE_CTX_free(verify_ctx);
	}

	return X509_STORE_CTX_get_error(verify_ctx);
}

static void dump_file(const char *path)
{
	FILE *f;
	char line[PATH_MAX];

	f = fopen(path, "r");
	if (!f) {
		return;
	}

	while (!feof(f)) {
		/* read next line */
		if (fgets(line, PATH_MAX, f) == NULL) {
			break;
		}

		print("%s", line)
	}

	fclose(f);
}

void signature_print_info(const char *path)
{
	X509 *cert;
	char *subj, *issuer;

	if (!path) {
		error("Invalid swupd certificate - Empty\n");
		return;
	}

	cert = get_cert_from_path(path);
	if (!cert) {
		error("Invalid swupd certificate - Error parsing the certificate\n");
		return;
	}

	subj = X509_NAME_oneline(X509_get_subject_name(cert), NULL, 0);
	issuer = X509_NAME_oneline(X509_get_issuer_name(cert), NULL, 0);
	print("Issuer: %s\n", subj);
	print("Subject: %s\n", subj);
	info("\n");

	dump_file(path);

	OPENSSL_free(subj);
	OPENSSL_free(issuer);

	X509_free(cert);
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

bool signature_verify(const char UNUSED_PARAM *file, const char UNUSED_PARAM *sig_file, enum signature_flags UNUSED_PARAM flags)
{
	return true;
}

void signature_print_info(const char UNUSED_PARAM *path)
{
	return;
}

bool signature_verify_data(const void UNUSED_PARAM *data, size_t UNUSED_PARAM data_len, const void UNUSED_PARAM *sig_data, size_t UNUSED_PARAM sig_data_len, enum signature_flags UNUSED_PARAM flags)
{
	return true;
}
#endif
