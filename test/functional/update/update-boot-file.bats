#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_version -p "$TEST_NAME" 100 10
	update_bundle "$TEST_NAME" os-core --add /usr/lib/kernel/testfile

}

@test "UPD007: Updating a system where a boot file was added in the newer version" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 100
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
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Warning: helper script ($TEST_DIRNAME/testfs/target-dir//usr/bin/clr-boot-manager) not found, it will be skipped
		Update successful - System updated from version 10 to version 100
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/lib/kernel/testfile

}
