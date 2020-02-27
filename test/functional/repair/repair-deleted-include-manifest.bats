#!/usr/bin/env bats

# Author: John Akre
# Email: john.w.akre@intel.com

load "../testlib"

test_setup() {

	test_setup_gen
}

@test "REP037: Report error when an included manifest is deleted" {

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS"

	assert_status_is "$SWUPD_COULDNT_LOAD_MANIFEST"
	expected_output=$(cat <<-EOM
		Diagnosing version 100
		Downloading missing manifests...
		Error: Failed to retrieve 10 test-bundle2 manifest
		Error: Unable to download manifest test-bundle2 version 10, exiting now
		Repair did not fully succeed
	EOM
	)
	assert_regex_is_output "$expected_output"

}
#WEIGHT=3
