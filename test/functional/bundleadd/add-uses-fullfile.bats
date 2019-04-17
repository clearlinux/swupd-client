#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo "$TEST_NAME"

}

@test "ADD018: When adding a bundle with less than 10 files, fullfiles should be used" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1"

	assert_status_is 0
	# validate file is installed in target
	assert_file_exists "$TARGETDIR"/foo
	# validate zero pack was not downloaded
	assert_file_not_exists "$STATEDIR"/pack-test-bundle1-from-0-to-10.tar
	expected_output=$(cat <<-EOM
		No packs need to be downloaded
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Installing bundle(s) files...
		Calling post-update helper scripts.
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

}
