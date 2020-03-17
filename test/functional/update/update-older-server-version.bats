#!/usr/bin/env bats

load "../testlib"

metadata_setup() {

	create_test_environment "$TEST_NAME" 10

}

@test "UPD019: Try updating a system where the server is older than the target system" {

	set_current_version "$TEST_NAME" 20

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
		Version on server (10) is not newer than system version (20)
		Update complete - System already up-to-date at version 20
	EOM
	)
	assert_is_output "$expected_output"

}

@test "UPD020: Try updating a system where there is no server version file available" {

	sudo rm -rf "$WEBDIR"/version

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is "$SWUPD_SERVER_CONNECTION_ERROR"
	expected_output=$(cat <<-EOM
		Error: Unable to determine the server version
		Update failed
	EOM
	)
	assert_in_output "$expected_output"

}

@test "UPD021: Try updating a system where there is no target version file available" {

	sudo rm -rf "$TARGETDIR"/usr/lib/os-release

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is "$SWUPD_CURRENT_VERSION_UNKNOWN"
	expected_output=$(cat <<-EOM
		Update started
		Error: Unable to determine current OS version
		Warning: Unable to determine current OS version
		Update failed
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=3
