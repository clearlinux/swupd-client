#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_version "$TEST_NAME" 100 10
	update_bundle "$TEST_NAME" os-core --add /test-file
	file_hash=$(get_hash_from_manifest "$TEST_NAME"/web-dir/100/Manifest.os-core /test-file)
	# copy the file from the files directory to state/staged and modify it
	sudo cp "$WEB_DIR"/100/files/"$file_hash" "$ABS_STAGED_DIR"
	write_to_protected_file -a "$ABS_STAGED_DIR"/"$file_hash" "extra string"

}

@test "UPD026: Update should discard full files with bad hash in the state dir" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

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
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 100
	EOM
	)
	# no errors are printed for a mismatched hash
	assert_is_output "$expected_output"
	# the file exists on the filesystem (it does not exist before the test)
	assert_file_exists "$TARGET_DIR"/test-file

}
#WEIGHT=2
