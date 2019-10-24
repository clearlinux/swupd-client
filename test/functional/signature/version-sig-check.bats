#!/usr/bin/env bats

# Author: Karthik Prabhu
# Email: karthik.prabhu.vinod@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /file_1 "$TEST_NAME"
	create_version -p "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" test-bundle1 --add file_3

}

@test "SIG019: Verify signature for absolute latest version with swupd check-update" {

	#TODO: Remove on FB 30
	skip "Skipping until FB 30 - feature disabled"
	# absolute latest version get checked during check-update
	sudo sh -c "mv $WEBDIR/version/latest_version.sig $WEBDIR/version/latest_version_bad_name.sig"
	run sudo sh -c "$SWUPD check-update $SWUPD_OPTS"
	assert_status_is "$SWUPD_SIGNATURE_VERIFICATION_FAILED"

	expected_output=$(cat <<-EOM
		Error: Failed to retrieve size for signature file: .*latest_version.sig
		Error: Signature Verification failed for URL: .*latest_version
		Error: Unable to determine the server version as signature verification failed
		Current OS version: 10
	EOM
	)
	assert_regex_is_output "$expected_output"

	sudo sh -c "mv $WEBDIR/version/latest_version_bad_name.sig $WEBDIR/version/latest_version.sig"
	# check-update succeeds as verification is successful
	run sudo sh -c "$SWUPD check-update $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"

	expected_output=$(cat <<-EOM
		Current OS version: 10
		Latest server version: 20
		There is a new OS version available: 20
	EOM
	)
	assert_is_output "$expected_output"

}

@test "SIG020: Verify fail signature for format staging with swupd update" {

	write_to_protected_file "$WEBDIR/version/formatstaging/latest.sig" "bad signature"

	# Update fails as signature match error occurs as expected
	run sudo sh -c "$SWUPD update $SWUPD_OPTS"
	assert_status_is "$SWUPD_SIGNATURE_VERIFICATION_FAILED"

	expected_output=$(cat <<-EOM
		Update started
		Error: Signature check error
		.*:not enough data.*
		Error: Unable to determine the server version as signature verification failed
		Update failed
	EOM
	)
	assert_regex_is_output "$expected_output"

}
