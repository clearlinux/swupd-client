#!/usr/bin/env bats

load "../testlib"

@test "check-update with no new version available" {

	run sudo sh -c "$SWUPD check-update $SWUPD_OPTS_NO_CERT"
	assert_status_is 1
	expected_output=$(cat <<-EOM
		Current OS version: 10
		There are no updates available
	EOM
	)
	assert_is_output "$expected_output"

}
