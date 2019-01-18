#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /test-file1 "$TEST_NAME"
	create_bundle -L -n test-bundle2 -f /test-file2 "$TEST_NAME"
	create_bundle -L -n test-bundle3 -f /test-file3 "$TEST_NAME"
	# add dependencies
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle2 test-bundle1
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle3 test-bundle2

}

@test "REM011: Try removing a bundle that is a nested dependency of another bundle" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1"

	assert_status_is "$SWUPD_REQUIRED_BUNDLE_ERROR"
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle3
	assert_file_exists "$TARGETDIR"/test-file1
	assert_file_exists "$TARGETDIR"/test-file2
	assert_file_exists "$TARGETDIR"/test-file3
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
