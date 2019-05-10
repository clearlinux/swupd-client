#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /usr/lib/kernel/testfile "$TEST_NAME"
	# change the content of testfile so the hash doesn't match
	write_to_protected_file -a "$TARGETDIR"/usr/lib/kernel/testfile "some new content"

}

@test "DIA004: Diagnose a system that has a corrupt boot file" {

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS"
	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Verifying version 10
		Verifying files
		Hash mismatch for file: .*/target-dir/usr/lib/kernel/testfile
		Inspected 7 files
		  1 file did not match
		Diagnose successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/lib/kernel/testfile

}
