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

	# absolute latest version get checked during check-update
	sudo sh -c "mv $WEB_DIR/version/latest_version.sig $WEB_DIR/version/latest_version_bad_name.sig"
	run sudo sh -c "$SWUPD check-update $SWUPD_OPTS"
	assert_status_is "$SWUPD_SIGNATURE_VERIFICATION_FAILED"

	expected_output=$(cat <<-EOM
		Error: Signature for latest file (file://$ABS_TEST_DIR/web-dir/version/latest_version) is missing
		Error: Unable to determine the server version as signature verification failed
		Current OS version: 10
	EOM
	)
	assert_is_output "$expected_output"

	sudo sh -c "mv $WEB_DIR/version/latest_version_bad_name.sig $WEB_DIR/version/latest_version.sig"
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

	write_to_protected_file "$WEB_DIR/version/formatstaging/latest.sig" "bad signature"

	# Update fails as signature match error occurs as expected
	run sudo sh -c "$SWUPD update $SWUPD_OPTS"
	assert_status_is "$SWUPD_SIGNATURE_VERIFICATION_FAILED"

	expected_output=$(cat <<-EOM
		Update started
		Error: Signature verification failed for URL: file://$ABS_TEST_DIR/web-dir/version/formatstaging/latest
		Error: Unable to determine the server version as signature verification failed
		Update failed
	EOM
	)
	assert_is_output "$expected_output"

}

@test "SIG021: Update using nosigcheck" {

	write_to_protected_file "$WEB_DIR/version/formatstaging/latest.sig" "bad signature"

	# Update fails as signature match error occurs as expected
	run sudo sh -c "$SWUPD update $SWUPD_OPTS --nosigcheck"
	assert_status_is "$SWUPD_OK"

	expected_output=$(cat <<-EOM
		Update started
		Warning: The --nosigcheck flag was used and this compromises the system security
		Warning: THE SIGNATURE OF file://$ABS_TEST_DIR/web-dir/version/formatstaging/latest WILL NOT BE VERIFIED
		Preparing to update from 10 to 20
		Warning: THE SIGNATURE OF file://$ABS_TEST_DIR/web-dir/10/Manifest.MoM WILL NOT BE VERIFIED
		Warning: THE SIGNATURE OF file://$ABS_TEST_DIR/web-dir/20/Manifest.MoM WILL NOT BE VERIFIED
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 0
		    new files         : 1
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 20
	EOM
	)
	assert_is_output "$expected_output"

}

@test "SIG022: Update using nosigcheck-latest" {

	write_to_protected_file "$WEB_DIR/version/formatstaging/latest.sig" "bad signature"

	# Update fails as signature match error occurs as expected
	run sudo sh -c "$SWUPD update $SWUPD_OPTS --nosigcheck-latest"
	assert_status_is "$SWUPD_OK"

	expected_output=$(cat <<-EOM
		Update started
		Warning: The --nosigcheck-latest flag was used and this compromises the system security
		Warning: THE SIGNATURE OF file://$ABS_TEST_DIR/web-dir/version/formatstaging/latest WILL NOT BE VERIFIED
		Preparing to update from 10 to 20
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 0
		    new files         : 1
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 20
	EOM
	)
	assert_is_output "$expected_output"

}

@test "SIG023: Fail update using nosigcheck-latest with an invalid MoM sig" {

	write_to_protected_file "$WEB_DIR/version/formatstaging/latest.sig" "bad signature"
	write_to_protected_file "$WEB_DIR/10/Manifest.MoM.sig"

	# Update fails as signature match error occurs as expected
	run sudo sh -c "$SWUPD update $SWUPD_OPTS --nosigcheck-latest"
	assert_status_is "$SWUPD_COULDNT_LOAD_MOM"

	expected_output=$(cat <<-EOM
		Update started
		Warning: The --nosigcheck-latest flag was used and this compromises the system security
		Warning: THE SIGNATURE OF file://$ABS_TEST_DIR/web-dir/version/formatstaging/latest WILL NOT BE VERIFIED
		Preparing to update from 10 to 20
		Warning: Signature check failed
		Warning: Removing corrupt Manifest.MoM artifacts and re-downloading...
		Warning: Signature check failed
		Error: Signature verification failed for manifest version 10
		Update failed
	EOM
	)
	assert_is_output "$expected_output"

}

#WEIGHT=13
