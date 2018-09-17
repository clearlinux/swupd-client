#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	# create a bundle with a boot file (in /usr/lib/kernel)
	create_bundle -n test-bundle -f /usr/lib/kernel/test-file "$TEST_NAME"

}

@test "bundle-add add bundle with --no-scripts" {

	run sudo sh -c "$SWUPD bundle-add --no-scripts $SWUPD_OPTS test-bundle"
	assert_status_is 0
	assert_file_exists "$TEST_NAME/target-dir/usr/lib/kernel/test-file"
	expected_output=$(cat <<-EOM
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Installing bundle(s) files...
		WARNING: post-update helper scripts skipped due to --no-scripts argument
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

}
