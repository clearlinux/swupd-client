#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_version "$TEST_NAME" 100 10
	update_bundle "$TEST_NAME" os-core --add /test-file
	file_hash=$(get_hash_from_manifest "$TEST_NAME"/web-dir/100/Manifest.os-core /test-file)
	# copy the file from the files directory to state/staged and modify it
	sudo cp "$WEBDIR"/100/files/"$file_hash" "$STATEDIR"/staged/
	write_to_protected_file -a "$STATEDIR"/staged/"$file_hash" "extra string"

}

@test "update full file with bad hash in state dir" {

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
		    changed files     : 0
		    new files         : 1
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
	# no errors are printed for a mismatched hash
	assert_is_output "$expected_output"
	# the file exists on the filesystem (it does not exist before the test)
	assert_file_exists "$TARGETDIR"/test-file

}
