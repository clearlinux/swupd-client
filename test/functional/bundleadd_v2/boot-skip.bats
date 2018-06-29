#!/usr/bin/env bats

load "../testlib"

setup() {

	create_test_environment "$TEST_NAME"
	# create a bundle with a boot file (in /usr/lib/kernel)
	create_bundle -n test-bundle -f /usr/lib/kernel/test-file "$TEST_NAME"

}

teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "bundle-add add bundle containing boot file with boot update override" {

	run sudo sh -c "$SWUPD bundle-add -b $SWUPD_OPTS test-bundle"

	assert_status_is 0
	assert_file_exists "$TEST_NAME/target-dir/usr/lib/kernel/test-file"
	expected_output=$(cat <<-EOM
		Starting download of remaining update content. This may take a while...

		File /usr/lib/kernel was not in a pack
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		WARNING: boot files update skipped due to --no-boot-update argument
		Successfully installed 1 bundle
	EOM
	)
	assert_in_output "$expected_output"

}
