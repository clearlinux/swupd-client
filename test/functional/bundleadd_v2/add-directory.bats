#!/usr/bin/env bats

load "../testlib"

setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -d /usr/bin/test "$TEST_NAME"

}

teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "bundle-add add bundle containing a directory" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

	assert_status_is 0
	assert_dir_exists "$TEST_NAME/target-dir/usr/bin/test"
	expected_output=$(cat <<-EOM
		Starting download of remaining update content. This may take a while...

		File /usr/bin/test was not in a pack
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		Successfully installed 1 bundle
	EOM
	)
	assert_in_output "$expected_output"

} 
