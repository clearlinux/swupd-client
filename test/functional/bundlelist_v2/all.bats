#!/usr/bin/env bats

load "../testlib"

setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo "$TEST_NAME"
	create_bundle -n test-bundle2 -f /bar "$TEST_NAME"

}

teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "bundle-list all bundles" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --all"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		os-core
		test-bundle1
		test-bundle2
	EOM
	)
	assert_in_output "$expected_output"

}
