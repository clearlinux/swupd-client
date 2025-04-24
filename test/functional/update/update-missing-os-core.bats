#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME"
	create_bundle -n os-core -f /os-core "$TEST_NAME"
	create_bundle -L -n test-bundle -f /foo/testfile2 "$TEST_NAME"
	create_version -p "$TEST_NAME" 100 10
	update_bundle "$TEST_NAME" os-core --add /testfile1
	update_bundle "$TEST_NAME" test-bundle --update /foo/testfile2

}

@test "UPD014: Update always adds os-core to tracked bundles" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 100 (in format staging)
		Downloading packs for:
		 - os-core
		 - test-bundle
		Finishing packs extraction...
		Statistics for going from version 10 to version 100:
		    changed bundles   : 2
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 1
		    new files         : 1
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 100
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGET_DIR"/testfile1

}
#WEIGHT=4
