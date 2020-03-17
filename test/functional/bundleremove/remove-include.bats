#!/usr/bin/env bats

load "../testlib"

metadata_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /foo/test-file1 "$TEST_NAME"
	create_bundle -L -n test-bundle2 -f /bar/test-file2 "$TEST_NAME"
	# add test-bundle1 as dependency of test-bundle2
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle2 test-bundle1

}

@test "REM010: Try removing a bundle that is a dependency of another bundle" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1"

	assert_status_is "$SWUPD_REQUIRED_BUNDLE_ERROR"
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/bar/test-file2
	expected_output=$(cat <<-EOM
		Bundle "test-bundle1" is required by the following bundles:
		 - test-bundle2
		Error: Bundle "test-bundle1" is required by 1 bundle, skipping it...
		Use "swupd bundle-remove --force test-bundle1" to remove "test-bundle1" and all bundles that require it
		Failed to remove 1 of 1 bundles
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=3
