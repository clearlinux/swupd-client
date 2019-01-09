#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -c /foo -f /foo/file1,/foo/file2,/bar/file3 "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" test-bundle --delete /foo/file1
	set_current_version "$TEST_NAME" 20

}

@test "VER042: Verify fixes a system with extra files that are unsafe to delete" {

	# when running verify --fix and there are extra files, but the extra
	# files are found not safe to be deleted, they should be skipped and
	# users should not be notified about these

	run sudo sh -c "$SWUPD verify --fix $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 20
		Verifying files
		Adding any missing files
		Fixing modified files
		Inspected 9 files
		Calling post-update helper scripts.
		Fix successful
	EOM
	)
	assert_is_output "$expected_output"

}
