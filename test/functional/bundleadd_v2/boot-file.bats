#!/usr/bin/env bats

load "../testlib"

setup() {

	create_test_environment "$TEST_NAME"
	# create a bundle with a boot file (in /usr/lib/kernel)
	create_bundle -n test-bundle -f /usr/lib/kernel/test-file "$TEST_NAME"

}

teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "bundle-add add bundle containing boot file" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"
	echo "Actual status: $status"
	echo "$output" >&3
	[ "$status" -eq 0 ]
	[ -f "$TEST_NAME/target-dir/usr/lib/kernel/test-file" ]
	# TODO(castulo): refactor and enable the check_lines function
	# check_lines "$output"

}
