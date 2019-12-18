#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /test-file "$TEST_NAME"
	sudo sh -c "openssl req -x509 -sha512 -days 1 -newkey rsa:4096  -keyout $TEST_NAME/key -out $TEST_NAME/cert -nodes -subj '/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost'"

}

@test "SIG005: Swupd bundle-add with invalid certificate" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS_NO_CERT -C $TEST_NAME/cert test-bundle"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Error: Signature verification failed for manifest version 10
		Error: Cannot load official manifest MoM for version 10
	EOM
	)
	assert_in_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/test-file

}

@test "SIG006: Force swupd bundle-add a bundle with invalid certificate" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS_NO_CERT -C $TEST_NAME/cert --nosigcheck test-bundle"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: The --nosigcheck flag was used, THE MANIFEST SIGNATURE WILL NOT BE VERIFIED
		Be aware that this compromises the system security
		Loading required manifests...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Calling post-update helper scripts
		Successfully installed 1 bundle
	EOM
	)
	assert_in_output "$expected_output"
	assert_file_exists "$TARGETDIR"/test-file

}
