#!/usr/bin/env bats

load "../testlib"

setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /usr/bin/file1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /media/lib/file2 "$TEST_NAME"

}

teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "bundle-add add multiple bundles" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1 test-bundle2"
	[ "$status" -eq 0 ]
	echo "Actual status: $status"
	[ -d "$TEST_NAME/target-dir/usr/bin" ]
	[ -f "$TEST_NAME/target-dir/usr/bin/file1" ]
	[ -d "$TEST_NAME/target-dir/media/lib" ]
	[ -f "$TEST_NAME/target-dir/media/lib/file2" ]
	echo "$output" >&3
	# TODO(castulo): refactor and enable the check_lines function
	# check_lines "$output"

}
