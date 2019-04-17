#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -e -n test-bundle1 -f /file_1,/foo/file_2 "$TEST_NAME"

}

test_teardown() {

	destroy_test_environment "$TEST_NAME"

}
@test "ADD032: Add an experimental bundle" {

	# Experimental bundles are added normally but the user gets warned about it

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Warning: Bundle test-bundle1 is experimental
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
