#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_version -p "$TEST_NAME" 100 10
	update_bundle "$TEST_NAME" os-core --add /foo

}

@test "UPD032: Download the update content only" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS --download"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 100 (in format staging)
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Statistics for going from version 10 to version 100:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 0
		    new files         : 1
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_not_exists "$TARGET_DIR"/foo
}

#WEIGHT=2
