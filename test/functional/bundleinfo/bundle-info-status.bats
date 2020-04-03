#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L    -n test-bundle1 -f /file_1 "$TEST_NAME"
	create_bundle -L -t -n test-bundle2 -f /file_2 "$TEST_NAME"

}

test_teardown() {

	# do nothing, just overwrite the lib test_setup
	return


}

global_teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "BIN013: Show info about a bundle installed in the system explicitly" {

	# bundles explicitly installed by the user should show that distinction

	run sudo sh -c "$SWUPD bundle-info $SWUPD_OPTS test-bundle2"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_______________________________
		 Info for bundle: test-bundle2
		_______________________________
		Status: Explicitly installed
		Bundle test-bundle2 is up to date:
		 - Installed bundle last updated in version: 10
		 - Latest available version: 10
		Bundle size:
		 - Size of bundle: .* KB
		 - Size bundle takes on disk \\(includes dependencies\\): .* KB
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "BIN014: Show info about a bundle installed in the system implicitly" {

	# bundles implicitly installed should just show as installed

	run sudo sh -c "$SWUPD bundle-info $SWUPD_OPTS test-bundle1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_______________________________
		 Info for bundle: test-bundle1
		_______________________________
		Status: Installed
		Bundle test-bundle1 is up to date:
		 - Installed bundle last updated in version: 10
		 - Latest available version: 10
		Bundle size:
		 - Size of bundle: .* KB
		 - Size bundle takes on disk \\(includes dependencies\\): .* KB
	EOM
	)
	assert_regex_is_output "$expected_output"

}
#WEIGHT=2
