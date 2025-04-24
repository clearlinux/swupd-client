#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_version -p "$TEST_NAME" 100 10
	update_bundle "$TEST_NAME" os-core --add /usr/lib/kernel/testfile

}

@test "UPD031: Updating a system where a boot file was added but not updated in the newer version" {

	run sudo sh -c "$SWUPD update --no-boot-update $SWUPD_OPTS"

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
		    new files         : 4
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Warning: boot files update skipped due to --no-boot-update argument
		Update successful - System updated from version 10 to version 100
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGET_DIR"/usr/lib/kernel/testfile

}
#WEIGHT=3
