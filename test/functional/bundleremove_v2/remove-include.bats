#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /foo/test-file1 "$TEST_NAME"
	create_bundle -L -n test-bundle2 -f /bar/test-file2 "$TEST_NAME"
	# add test-bundle1 as dependency of test-bundle2
	add_dependency_to_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle2 test-bundle1

}

@test "bundle-remove remove bundle containing a file" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1"
	assert_status_is "$EBUNDLE_REMOVE"
	assert_file_exists "$TEST_NAME"/target-dir/foo/test-file1
	assert_file_exists "$TEST_NAME"/target-dir/bar/test-file2
	expected_output=$(cat <<-EOM
		Error: bundle requested to be removed is required by the following bundles:
		format:
		 # * is-required-by
		 #   |-- is-required-by
		 # * is-also-required-by
		 # ...
		  * test-bundle2
		Failed to remove 1 of 1 bundles
	EOM
	)
	assert_is_output "$expected_output"

}
