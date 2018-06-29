#!/usr/bin/env bats

load "../testlib"

setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /usr/bin/file1 "$TEST_NAME"

}

teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "bundle-add an already existing bundle" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

	assert_status_is 18
	expected_output=$(cat <<-EOM
		Warning: Bundle "test-bundle" is already installed, skipping it...
		1 bundle was already installed
	EOM
	)
	assert_in_output "$expected_output"

}
