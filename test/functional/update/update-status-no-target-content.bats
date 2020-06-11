#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME"
	create_version "$TEST_NAME" 100
	sudo rm -f "$TARGET_DIR"/usr/lib/os-release

}

@test "UPD038: Try showing current OS version when there is no target content" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS --status"

	assert_status_is "$SWUPD_CURRENT_VERSION_UNKNOWN"
	expected_output=$(cat <<-EOM
		Error: Unable to determine current OS version
		Latest server version: 100
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=1
