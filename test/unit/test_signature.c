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
			   file_sig);

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
	char *private_key, *public_key;
	int ret;

	private_key = str_or_die("%s/private.pem", test_dir);
	public_key = str_or_die("%s/public.pem", test_dir);

	// Create a good key
	ret = run_command("/usr/bin/openssl", "req", "-x509", "-sha256",
			  "-nodes", "-days", "365", "-newkey", "rsa:4096",
			  "-keyout", private_key, "-out", public_key,
			  "-subj", "/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost", NULL);
	if (ret != 0) {
		return -1;
	}

	ret = test_sign(private_key, public_key, true);

	//TODO: Add tests with invalid signatures - expired, OCSP

	free(private_key);
	free(public_key);

	return ret;
}

int main()
{

	if (set_up() < 0) {
		print_error("Test set_up() error");
		goto error;
	}

	if (test_signature() < 0) {
		goto error;
	}

	tear_down();

	return 0;

error:
	return EXIT_FAILURE;
}
