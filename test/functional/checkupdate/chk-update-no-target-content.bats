#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	# remove os-release file from target-dir so no current version can be determined
	sudo rm "$TEST_NAME"/target-dir/usr/lib/os-release

}

@test "CHK005: Check for available updates with no target version file available" {

	run sudo sh -c "$SWUPD check-update $SWUPD_OPTS"

	assert_status_is_not 0
	assert_is_output "Error: Unable to determine current OS version"

}
