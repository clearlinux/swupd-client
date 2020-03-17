#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

metadata_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo/file_1 "$TEST_NAME"
	create_version -p "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" test-bundle1 --add /bar/file_2
	set_current_version "$TEST_NAME" 10

}

@test "SRH030: Search for a file specifying the version" {

	# Users should be able to search for files in specific versions of Clear
	# using the --version option

	run sudo sh -c "$SWUPD search-file $SWUPD_OPTS file_2"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Downloading all Clear Linux manifests
		Searching for 'file_2'
		Search term not found
	EOM
	)
	assert_is_output "$expected_output"

	run sudo sh -c "$SWUPD search-file $SWUPD_OPTS file_2 --version 20"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Downloading all Clear Linux manifests
		Searching for 'file_2'
		Bundle test-bundle1 \\(0 MB to install\\)
		./bar/file_2
	EOM
	)
	assert_regex_is_output "$expected_output"

}
#WEIGHT=3
