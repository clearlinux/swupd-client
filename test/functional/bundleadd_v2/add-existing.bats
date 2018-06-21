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
	[ "$status" -eq 18 ]
	echo "Actual status: $status"
	echo "$output" >&3
	# TODO(castulo): refactor and enable the check_lines function
	# check_lines "$output"

}
