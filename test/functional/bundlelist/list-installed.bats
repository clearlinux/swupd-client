#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /file_1 "$TEST_NAME"
	create_bundle -L -n test-bundle2 -f /file_2 "$TEST_NAME"
	create_bundle -n test-bundle3 -f /file_3 "$TEST_NAME"

}

@test "LST001: List all installed bundles" {

	# bundle-list with no options should list only installed bundles

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Installed bundles:
		 - os-core
		 - test-bundle1
		 - test-bundle2

		Total: 3
	EOM
	)
	assert_is_output --identical "$expected_output"

}
