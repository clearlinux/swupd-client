#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /usr/lib/kernel/testfile "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" test-bundle --delete /usr/lib/kernel/testfile
	set_current_version "$TEST_NAME" 20

}

@test "VER017: Verify can skip running the post-update scripts and boot update tool" {

	run sudo sh -c "$SWUPD verify --fix --no-scripts $SWUPD_OPTS"
	assert_status_is 0
	# check for the warning
	expected_output=$(cat <<-EOM
		Verifying version 20
		Verifying files
		Adding any missing files
		Fixing modified files
		Inspected 7 files
		Warning: post-update helper scripts skipped due to --no-scripts argument
		Fix successful
	EOM
	)
	assert_is_output "$expected_output"
	# this should exist at the end, even if the scripts are not run
	assert_file_exists "$TARGETDIR"/usr/lib/kernel/testfile

}
