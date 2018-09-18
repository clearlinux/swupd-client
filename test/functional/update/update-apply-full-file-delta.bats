#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_version -p "$TEST_NAME" 100 10
	update_bundle "$TEST_NAME" os-core --update /core

}

@test "update use delta pack" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started.
		Preparing to update from 10 to 100
		Downloading packs...
		Extracting os-core pack for version 100
		Statistics for going from version 10 to version 100:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 1
		    new files         : 0
		    deleted files     : 0
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Staging file content
		Applying update
		Update was applied.
		Calling post-update helper scripts.
		Update successful. System updated from version 10 to version 100
	EOM
	)
	assert_is_output "$expected_output"
	# make sure the file was updated correctly
	fhash=$(get_hash_from_manifest "$WEBDIR"/100/Manifest.os-core /core)
	assert_files_equal "$TARGETDIR"/core "$TEST_NAME"/web-dir/100/files/"$fhash"

}
