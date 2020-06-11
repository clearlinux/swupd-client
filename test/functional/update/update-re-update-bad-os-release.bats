#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME" 10 1
	bump_format "$TEST_NAME"
	create_version -r "$TEST_NAME" 40 30 2
	# remove the os-release file from all manifests
	remove_from_manifest "$WEB_DIR"/10/Manifest.os-core /usr/lib/os-release
	remove_from_manifest "$WEB_DIR"/20/Manifest.os-core /usr/lib/os-release
	remove_from_manifest "$WEB_DIR"/30/Manifest.os-core /usr/lib/os-release
	remove_from_manifest "$WEB_DIR"/40/Manifest.os-core /usr/lib/os-release

}

@test "UPD024: Try updating a system across a format bump with a bad os-release file" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS_NO_FMT"

	assert_status_is "$SWUPD_CURRENT_VERSION_UNKNOWN"
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
		    changed files     : 1
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 20
		Error: Inconsistency between version files, exiting now
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGET_DIR"/core

}
#WEIGHT=8
