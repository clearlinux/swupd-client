#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /file_1 "$TEST_NAME"

	create_version "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" test-bundle1 --add /file_2

	# create a mirror at this point
	create_mirror "$TEST_NAME"
	update_bundle "$TEST_NAME" test-bundle1 --add /file_3

}

@test "UPD061: Updating a system using mirror without latest signature" {

	sudo rm "$ABS_MIRROR_DIR"/version/formatstaging/latest.sig
	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Update started
		Checking mirror status
		Error: Signature for latest file (file://$ABS_MIRROR_DIR/version/formatstaging/latest) is missing
		Warning: the mirror version could not be determined
		Removing mirror configuration
		Preparing to update from 10 to 20 (in format staging)
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 0
		    new files         : 2
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
	assert_file_exists "$TARGET_DIR"/file_2
	assert_file_exists "$TARGET_DIR"/file_3

}

@test "UPD062: Updating a system using mirror with invalid latest signature" {

	write_to_protected_file "$ABS_MIRROR_DIR"/version/formatstaging/latest.sig "1234"
	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Update started
		Checking mirror status
		Error: Signature verification failed for URL: file://$ABS_MIRROR_DIR/version/formatstaging/latest
		Warning: the mirror version could not be determined
		Removing mirror configuration
		Preparing to update from 10 to 20 (in format staging)
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 0
		    new files         : 2
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
	assert_file_exists "$TARGET_DIR"/file_2
	assert_file_exists "$TARGET_DIR"/file_3

}
#WEIGHT=7
