#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

metadata_setup() {

	create_test_environment "$TEST_NAME"
	create_version -p "$TEST_NAME" 20 10

}

@test "UPD050: Update to the same version using -m" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS -m 10"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
		Requested version (10) is not newer than system version (10)
		Update complete - System already up-to-date at version 10
	EOM
	)
	assert_is_output "$expected_output"

}

@test "UPD051: Update to same version using -m, when already in last version" {

	set_current_version "$TEST_NAME" 20

	run sudo sh -c "$SWUPD update $SWUPD_OPTS -m 20"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
		Requested version (20) is not newer than system version (20)
		Update complete - System already up-to-date at version 20
	EOM
	)
	assert_is_output "$expected_output"

}

@test "UPD052: Update when already in last version" {

	set_current_version "$TEST_NAME" 20

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
		Version on server (20) is not newer than system version (20)
		Update complete - System already up-to-date at version 20
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=3
