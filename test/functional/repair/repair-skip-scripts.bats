#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /usr/lib/kernel/testfile "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" test-bundle --delete /usr/lib/kernel/testfile
	set_current_version "$TEST_NAME" 20

}

@test "REP030: Repair can skip running the post-update scripts and boot update tool" {

	run sudo sh -c "$SWUPD repair --no-scripts $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	# check for the warning
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Verifying files
		Adding any missing files
		Repairing modified files
		Inspected 7 files
		Warning: post-update helper scripts skipped due to --no-scripts argument
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"
	# this should exist at the end, even if the scripts are not run
	assert_file_exists "$TARGETDIR"/usr/lib/kernel/testfile

}
