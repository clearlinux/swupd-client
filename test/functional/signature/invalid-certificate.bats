#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

metadata_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /test-file "$TEST_NAME"

}

test_setup() {

	sudo sh -c "openssl req -x509 -sha512 -days 1 -newkey rsa:4096  -keyout $TEST_NAME/key -out $TEST_NAME/cert -nodes -subj '/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost'"

}

@test "SIG005: Swupd bundle-add with invalid certificate" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS_NO_CERT -C $TEST_NAME/cert test-bundle"

	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"
	expected_output=$(cat <<-EOM
		Warning: Signature check failed
		Warning: Removing corrupt Manifest.MoM artifacts and re-downloading...
		Warning: Signature check failed
		Error: Signature verification failed for manifest version 10
		Error: Cannot load official manifest MoM for version 10
		Failed to install 1 of 1 bundles
	EOM
	)
	assert_is_output "$expected_output"
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
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/test-file

}
#WEIGHT=4
