#!/usr/bin/env bats

load "../testlib"

setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo/test-file1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /bar/test-file2 "$TEST_NAME"
	# add test-bundle2 as a dependency of test-bundle1
	add_dependency_to_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle1 test-bundle2
	# since we modified one manifest we need to update that in MoM too, so re add the bundle manifest
	remove_from_manifest "$TEST_NAME"/web-dir/10/Manifest.MoM test-bundle1
	add_to_manifest	"$TEST_NAME"/web-dir/10/Manifest.MoM "$TEST_NAME"/web-dir/10/Manifest.test-bundle1 test-bundle1

}

teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "bundle-add verify include support" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1"
	echo "Actual status: $status"
	echo "$output" >&3
	[ "$status" -eq 0 ]
	[ -f "$TEST_NAME"/target-dir/foo/test-file1 ]
	[ -f "$TEST_NAME"/target-dir/bar/test-file2 ]
	# TODO(castulo): refactor and enable the check_lines function
	# check_lines "$output"

}
