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
	# add test-bundle2 as a dependency of test-bundle1 and test-bundle3 as optional
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle1 test-bundle2
	add_dependency_to_manifest -o "$WEBDIR"/10/Manifest.test-bundle1 test-bundle3
	# add test-bundle4 as a dependency of test-bundle2
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle2 test-bundle4
	# add one more dependency in a new version
	create_version "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" test-bundle1 --header-only
	create_bundle -n test-bundle5 -f /file_5 "$TEST_NAME"
	add_dependency_to_manifest "$WEBDIR"/20/Manifest.test-bundle1 test-bundle5

}

test_setup() {

	# do nothing, just overwrite the lib test_setup
	return

}

test_teardown() {

	# do nothing, just overwrite the lib test_setup
	return

}

global_teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "BIN006: Show info about a bundle including its dependencies" {

	# the --includes flag can be used to show all directly and indirectly
	# included and optional bundles

	run sudo sh -c "$SWUPD bundle-info $SWUPD_OPTS test-bundle1 --dependencies"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_______________________________
		 Info for bundle: test-bundle1
		_______________________________
		Status: Not installed
		Latest available version: 20
		Bundle size:
		 - Size of bundle: .* KB
		 - Maximum amount of disk size the bundle will take if installed \(includes dependencies\): .* KB
		Direct dependencies \(2\):
		 - test-bundle2
		 - test-bundle3 \(optional\)
		Indirect dependencies \(1\):
		 - test-bundle4
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "BIN007: Show info about a specific version of bundle including its dependencies" {

	# the --includes flag can be used to show all directly and indirectly
	# included and optional bundles along with the --version flag

	run sudo sh -c "$SWUPD bundle-info $SWUPD_OPTS test-bundle1 --dependencies --version latest"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_______________________________
		 Info for bundle: test-bundle1
		_______________________________
		Status: Not installed
		Latest available version: 20
		Bundle size:
		 - Size of bundle: .* KB
		 - Maximum amount of disk size the bundle will take if installed \(includes dependencies\): .* KB
		Direct dependencies \(3\):
		 - test-bundle2
		 - test-bundle5
		 - test-bundle3 \(optional\)
		Indirect dependencies \(1\):
		 - test-bundle4
	EOM
	)
	assert_regex_is_output "$expected_output"

}
#WEIGHT=6
