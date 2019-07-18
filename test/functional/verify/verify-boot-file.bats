#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /usr/lib/kernel/testfile "$TEST_NAME"
	# change the content of testfile so the hash doesn't match
	write_to_protected_file -a "$TARGETDIR"/usr/lib/kernel/testfile "some new content"

}

@test "VER004: Verify a system that has a corrupt boot file" {

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 10
		Verifying files
		Hash mismatch for file: .*/target-dir/usr/lib/kernel/testfile
		Inspected 7 files
		  1 file did not match
		Verify successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/lib/kernel/testfile

}

@test "VER005: Verify fixes a system that has a corrupt boot file" {

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --fix"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 10
		Verifying files
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Adding any missing files
		Fixing modified files
		Hash mismatch for file: .*/target-dir/usr/lib/kernel/testfile
		.fixed
		Inspected 7 files
		  1 file did not match
		    1 of 1 files were fixed
		    0 of 1 files were not fixed
		Calling post-update helper scripts.
		Warning: post-update helper script \\($TEST_DIRNAME/testfs/target-dir//usr/bin/clr-boot-manager\\) not found, it will be skipped
		Fix successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/lib/kernel/testfile

}