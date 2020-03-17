#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

metadata_setup() {

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
		Loading required manifests...
		Warning: Bundle test-bundle1 is experimental
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Calling post-update helper scripts
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=2
