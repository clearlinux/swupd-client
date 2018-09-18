#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -u "$TEST_NAME" 10 1
	bump_format "$TEST_NAME"
	create_version -u "$TEST_NAME" 40 30 2
	update_bundle "$TEST_NAME" os-core --add-dir /usr/bin

}

@test "verify format mismatch override" {

	# the -x option bypasses the fatal error
	run sudo sh -c "$SWUPD verify --fix -F 1 -m 40 -x $SWUPD_OPTS_NO_FMT"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 40
		WARNING: the force option is specified; ignoring format mismatch for verify
		WARNING: the force or picky option is specified; ignoring version mismatch for verify --fix
		Verifying files
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Adding any missing files
		Missing file: .+/target-dir/usr/bin
		.fixed
		Fixing modified files
		.Hash mismatch for file: .*/target-dir/usr/lib/os-release
		.fixed
		Hash mismatch for file: .*/target-dir/usr/share/defaults/swupd/format
		.fixed
		Inspected 10 files
		  1 files were missing
		    1 of 1 missing files were replaced
		    0 of 1 missing files were not replaced
		  2 files did not match
		    2 of 2 files were fixed
		    0 of 2 files were not fixed
		  0 files found which should be deleted
		Calling post-update helper scripts.
		Fix successful
	EOM
	)
	assert_regex_is_output "$expected_output"

}