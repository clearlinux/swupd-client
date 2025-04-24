#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /file_1,/foo/file_2 "$TEST_NAME"
	create_version -p "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" test-bundle1 --add /bar/file_3
	create_version -p "$TEST_NAME" 30 20
	update_bundle "$TEST_NAME" test-bundle1 --add /bar/file_b3

	create_delta_manifest test-bundle1 20 10
	sudo mv "$WEB_DIR"/20/Manifest-test-bundle1-delta-from-10 "$WEB_DIR"/30/Manifest-test-bundle1-delta-from-10
	write_to_protected_file -a "$WEB_DIR"/20/Manifest-test-bundle1-delta-from-10 "Invalid delta"

}

@test "UPD064: Update successful even when delta manifest is corrupted" {

	# Successfully update even when delta manifest is corrupted

	run sudo sh -c "$SWUPD update $SWUPD_OPTS -V 20"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
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

}

@test "UPD065: Update successful even when delta manifest is wrong" {

	# Successfully update even when delta manifest is generating an incorrect manifest

	run sudo sh -c "$SWUPD update $SWUPD_OPTS -V 30"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 30 (in format staging)
		Warning: hash check failed for Manifest.test-bundle1 for version 30. Deleting it
		Warning: Removing corrupt Manifest.test-bundle1 artifacts and re-downloading...
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 30:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 0
		    new files         : 3
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 30
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=8
