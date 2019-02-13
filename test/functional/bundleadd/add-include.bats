#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo/test-file1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /bar/test-file2 "$TEST_NAME"
	create_bundle -n test-bundle3 -f /bar/test-file3 "$TEST_NAME"
	# add test-bundle2 as a dependency of test-bundle1
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle1 test-bundle2

}

@test "ADD013: Adding a bundle that includes another bundle" {

	# Add bundle 3 to prime the bundles statedir contents so the tracked
	# bundle detection can work in a realistic way
	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle3"
	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1"

	assert_status_is 0
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/bar/test-file2
	assert_file_exists "$STATEDIR"/bundles/test-bundle1
	assert_file_exists "$STATEDIR"/bundles/test-bundle3
	assert_file_not_exists "$STATEDIR"/bundles/test-bundle2
	expected_output=$(cat <<-EOM
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Installing bundle(s) files...
		Calling post-update helper scripts.
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

}

@test "ADD048: Adding a bundle that includes another bundle (no tracking state)" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1"

	assert_status_is 0
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/bar/test-file2
	assert_file_exists "$STATEDIR"/bundles/test-bundle1
	# When no tracking directly previously existed, all currently installed
	# bundles are added to the tracked state.
	assert_file_exists "$STATEDIR"/bundles/test-bundle2
	expected_output=$(cat <<-EOM
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Installing bundle(s) files...
		Calling post-update helper scripts.
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

}
