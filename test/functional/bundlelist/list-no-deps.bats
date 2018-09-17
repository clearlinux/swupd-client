#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo "$TEST_NAME"
	create_bundle -n test-bundle2 -f /foo "$TEST_NAME"

}

@test "bundle-list list bundle deps with no included bundles" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --deps test-bundle1"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		No included bundles
	EOM
	)
	assert_is_output "$expected_output"

}
