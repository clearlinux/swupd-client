#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /test-file "$TEST_NAME"
    write_to_protected_file $TEST_NAME/cert "invalid"

}

@test "SIG001: Swupd bundle-add with corrupted certificate" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS_NO_CERT -C $TEST_NAME/cert test-bundle"

	assert_status_is "$SWUPD_SIGNATURE_VERIFICATION_FAILED"
	expected_output=$(cat <<-EOM
		Error: Failed to verify certificate: .*
	EOM
	)
	assert_regex_in_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/test-file

}

@test "SIG002: Force swupd bundle-add a bundle with corrupted certificate" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS_NO_CERT -C $TEST_NAME/cert --nosigcheck test-bundle"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		FAILED TO VERIFY SIGNATURE OF Manifest.MoM. Operation proceeding due to
		  --nosigcheck, but system security may be compromised
		Loading required manifests...
		No packs need to be downloaded
		Starting download of remaining update content. This may take a while...
		Installing bundle(s) files...
		Calling post-update helper scripts
		Successfully installed 1 bundle
	EOM
	)
	assert_in_output "$expected_output"
	assert_file_exists "$TARGETDIR"/test-file

}
