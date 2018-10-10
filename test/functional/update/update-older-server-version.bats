#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME" 10

}

@test "update with a server older than the target system" {

	set_current_version "$TEST_NAME" 20

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started.
		Version on server (10) is not newer than system version (20)
		Update complete. System already up-to-date at version 20
	EOM
	)
	assert_is_output "$expected_output"

}

@test "update with no server version file available" {

	sudo rm -rf "$WEBDIR"/version

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is 1
	expected_output=$(cat <<-EOM
		Update started.
		Error: Unable to determine the server version
		Update failed.
	EOM
	)
	assert_is_output "$expected_output"

}

@test "update with no target version file available" {

	sudo rm -rf "$TARGETDIR"/usr/lib/os-release

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is 1
	expected_output=$(cat <<-EOM
		Update started.
		Error: Unable to determine current OS version
		Unable to determine current OS version
		Update failed.
	EOM
	)
	assert_is_output "$expected_output"

}
