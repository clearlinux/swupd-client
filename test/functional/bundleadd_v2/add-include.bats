#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo/test-file1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /bar/test-file2 "$TEST_NAME"
	# add test-bundle2 as a dependency of test-bundle1
	add_dependency_to_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle1 test-bundle2
	# since we modified one manifest we need to update that in MoM too, so re add the bundle manifest
	update_hashes_in_mom "$TEST_NAME"/web-dir/10/Manifest.MoM

}

@test "bundle-add verify include support" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1"

	assert_status_is 0
	assert_file_exists "$TEST_NAME"/target-dir/foo/test-file1
	assert_file_exists "$TEST_NAME"/target-dir/bar/test-file2
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
