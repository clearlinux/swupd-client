#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "../../src/lib/sys.h"
#include "../../src/signature.h"
#include "../../src/lib/strings.h"
#include "test_helper.h"

static char test_dir[] = "./test_signature-XXXXXX";

static int set_up()
{
	if (mkdtemp(test_dir) == NULL) {
		return -1;
	}

	return 0;
}

static int tear_down()
{
	rm_rf(test_dir);

	return 0;
}

static int sign(const char *private_key, const char *public_key, const char *file, const char *file_sig)
{
	int ret;

	ret = run_command("/usr/bin/openssl", "smime", "-sign",
			   "-binary", "-in", file, "-signer", public_key,
			   "-inkey", private_key, "-outform", "DER", "-out",
			   file_sig, NULL);

	if (ret != 0) {
		print_error("Sign failed");
		return -1;
	}

	return 0;
}

static int test_sign(const char *private_key, const char *public_key, bool success)
{
	char *sig = NULL;
	char *file = NULL;
	int ret = -1;

	sig = str_or_die("%s/sig", test_dir);
	file = __FILE__;

	if (sign(private_key, public_key, file, sig) < 0) {
		goto error;
	}

	if (!signature_init(public_key, NULL)) {
		if (!success) { // Error expected
			ret = 0;
		}
		goto error;
	}

	// Check if signature match
	check_goto(signature_verify(file, sig, false) == success, error);

	// Check signature against wrong file
	check_goto(signature_verify(sig, sig, false) == false, error);

	ret = 0;

error:
	signature_deinit();
	free(sig);
	return ret;
}

static int test_signature()
{
	char *private_key, *public_key, *dir, *config_ocsp,
	     *config_ocsp_critical, *config_noocsp_critical;
	int result = -1, ret;

	private_key = sys_path_join(test_dir, "private.pem");
	public_key = sys_path_join(test_dir, "public.pem");
	dir = sys_dirname(__FILE__);
	config_ocsp = sys_path_join(dir, "data/config_ocsp.cnf");
	config_noocsp_critical = sys_path_join(dir, "data/config_noocsp_critical.cnf");
	config_ocsp_critical = sys_path_join(dir, "data/config_ocsp_critical.cnf");

	// Create a good key
	printf("Testing with a valid RSA certificate\n");
	rm_rf(private_key);
	rm_rf(public_key);
	ret = run_command("/usr/bin/openssl", "req", "-x509", "-sha256",
			  "-nodes", "-days", "365", "-newkey", "rsa:4096",
			  "-keyout", private_key, "-out", public_key,
			  "-subj", "/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost", NULL);
	if (ret != 0) {
		goto error;
	}

	ret = test_sign(private_key, public_key, true);
	if (ret != 0) {
		goto error;
	}

	// OCSP not critical - Should work
	printf("Testing with a certificate with OCSP support\n");
	rm_rf(private_key);
	rm_rf(public_key);
	ret = run_command("/usr/bin/openssl", "req", "-x509", "-sha256",
			  "-nodes", "-days", "365", "-newkey", "rsa:4096",
			  "-keyout", private_key, "-out", public_key,
			  "-config", config_ocsp,
			  "-subj", "/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost", NULL);
	if (ret != 0) {
		goto error;
	}

	ret = test_sign(private_key, public_key, true);
	if (ret != 0) {
		goto error;
	}

	// OCSP not critical - Should fail
	printf("Testing with a certificate with with OCSP support\n");
	rm_rf(private_key);
	rm_rf(public_key);
	ret = run_command("/usr/bin/openssl", "req", "-x509", "-sha256",
			  "-nodes", "-days", "365", "-newkey", "rsa:4096",
			  "-keyout", private_key, "-out", public_key,
			  "-config", config_ocsp_critical,
			  "-subj", "/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost", NULL);
	if (ret != 0) {
		goto error;
	}

	ret = test_sign(private_key, public_key, false);
	if (ret != 0) {
		goto error;
	}

	// NO OCSP critical - Should fail
	printf("Testing with a certificate without OCSP support, but critical\n");
	rm_rf(private_key);
	rm_rf(public_key);
	ret = run_command("/usr/bin/openssl", "req", "-x509", "-sha256",
			  "-nodes", "-days", "365", "-newkey", "rsa:4096",
			  "-keyout", private_key, "-out", public_key,
			  "-config", config_noocsp_critical,
			  "-subj", "/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost", NULL);
	if (ret != 0) {
		goto error;
	}

	ret = test_sign(private_key, public_key, false);
	if (ret != 0) {
		goto error;
	}


	result = 0;
error:
	// Release resources
	free(private_key);
	free(public_key);
	free(dir);
	free(config_ocsp);
	free(config_ocsp_critical);
	free(config_noocsp_critical);

	return result;
}

int main()
{
	int result = EXIT_FAILURE;

	if (set_up() < 0) {
		print_error("Test set_up() error");
		goto error;
	}

	if (test_signature() < 0) {
		goto error;
	}


	result = 0;
error:
	tear_down();
	return result;
}
