#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME"
	set_latest_version "$TEST_NAME" 100
	sudo rm -f "$TARGETDIR"/usr/lib/os-release

}

@test "update --status with no target content" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS --status"
	assert_status_is 2
	expected_output=$(cat <<-EOM
		Cannot determine current OS version
		Latest server version: 100
	EOM
	)
	assert_is_output "$expected_output"

}
