#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	# remove os-release file from target-dir so no current version can be determined
	sudo rm "$TARGET_DIR"/usr/lib/os-release

}

@test "CHK005: Check for available updates with no target version file available" {

	run sudo sh -c "$SWUPD check-update $SWUPD_OPTS"

	assert_status_is_not 0
	expected_output=$(cat <<-EOM
		Error: Unable to determine current OS version
		Latest server version: 10
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=1
