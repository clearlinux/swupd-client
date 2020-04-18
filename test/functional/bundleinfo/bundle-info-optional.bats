#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /file_1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /file_2 "$TEST_NAME"
	create_bundle -n test-bundle3 -f /file_3 "$TEST_NAME"
	create_bundle -n test-bundle4 -f /file_4 "$TEST_NAME"
	create_bundle -n test-bundle5 -f /file_5 "$TEST_NAME"
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle1 test-bundle2
	add_dependency_to_manifest -o "$WEBDIR"/10/Manifest.test-bundle1 test-bundle3
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle2 test-bundle4
	add_dependency_to_manifest -o "$WEBDIR"/10/Manifest.test-bundle2 test-bundle5
}

@test "BIN011: Show info about a bundle including its optional and non-optional dependencies" {

	run sudo sh -c "$SWUPD bundle-info $SWUPD_OPTS test-bundle1 --dependencies"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_______________________________
		 Info for bundle: test-bundle1
		_______________________________
		Status: Not installed
		Latest available version: 10
		Bundle size:
		 - Size of bundle: .* KB
		 - Maximum amount of disk size the bundle will take if installed \\(includes dependencies\\): .* KB
		Direct dependencies \\(3\\):
		 - os-core
		 - test-bundle2
		 - test-bundle3 \\(optional\\)
		Indirect dependencies \\(2\\):
		 - test-bundle4
		 - test-bundle5 \\(optional\\)
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "BIN012: Show info about a bundle including its installed optional and non-optional dependencies" {

	install_bundle "$WEBDIR"/10/Manifest.test-bundle1
	install_bundle "$WEBDIR"/10/Manifest.test-bundle2
	install_bundle "$WEBDIR"/10/Manifest.test-bundle5

	run sudo sh -c "$SWUPD bundle-info $SWUPD_OPTS test-bundle1 --dependencies"

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
		Direct dependencies \\(3\\):
		 - os-core
		 - test-bundle2
		 - test-bundle3 \\(optional, not installed\\)
		Indirect dependencies \\(2\\):
		 - test-bundle4
		 - test-bundle5 \\(optional, installed\\)
	EOM
	)
	assert_regex_is_output "$expected_output"

}
#WEIGHT=7
