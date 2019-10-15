#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME" 10 1
	bump_format "$TEST_NAME"
	create_version -r "$TEST_NAME" 40 30 2

}

@test "UPD023: Updating a system across a format bump" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS_NO_FMT"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 20
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 2
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 20
		Update started
		Preparing to update from 30 to 40
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Statistics for going from version 30 to version 40:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 2
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 30 to version 40
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/core

}
