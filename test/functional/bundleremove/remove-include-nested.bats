#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /test-file1 "$TEST_NAME"
	create_bundle -L -n test-bundle2 -f /test-file2 "$TEST_NAME"
	create_bundle -L -n test-bundle3 -f /test-file3 "$TEST_NAME"
	# add dependencies
	add_dependency_to_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle2 test-bundle1
	add_dependency_to_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle3 test-bundle2

}

@test "bundle-remove remove bundle with nested dependent bundles" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1"
	assert_status_is "$EBUNDLE_REMOVE"
	assert_file_exists "$TEST_NAME"/target-dir/test-file1
	assert_file_exists "$TEST_NAME"/target-dir/test-file2
	assert_file_exists "$TEST_NAME"/target-dir/test-file3
	expected_output=$(cat <<-EOM
		Error: bundle requested to be removed is required by the following bundles:
		format:
		 # * is-required-by
		 #   |-- is-required-by
		 # * is-also-required-by
		 # ...
		  * test-bundle2
		    |-- test-bundle3
		Failed to remove 1 of 1 bundles
	EOM
	)
	assert_is_output "$expected_output"

}
