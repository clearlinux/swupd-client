#!/usr/bin/env bats

load "../testlib"

@test "bundle-list list bundle deps with invalid bundle name" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --deps not-a-bundle"
	assert_status_is 1
	expected_output=$(cat <<-EOM
		Warning: Bundle "not-a-bundle" is invalid, skipping it...
		Error: Bad bundle name detected - Aborting
	EOM
	)
	assert_is_output "$expected_output"

}
