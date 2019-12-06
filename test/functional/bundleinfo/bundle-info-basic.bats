#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /file_1,/foo/file_2 "$TEST_NAME"
	create_bundle -e -n test-bundle2 -f /file_3,/bar/file_4 "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" test-bundle1 --update /file_1
	create_version "$TEST_NAME" 30 20
	update_bundle "$TEST_NAME" test-bundle1 --add /foo/file_5

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

@test "BIN001: Show info about a bundle installed in the system" {

	# users can use the bundle-info command to see detailed info about a bundle
	# if the bundle is installed it should show when it was last updated and
	# how much disk space is taking

	run sudo sh -c "$SWUPD bundle-info $SWUPD_OPTS test-bundle1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_______________________________
		 Info for bundle: test-bundle1
		_______________________________
		Status: Installed
		There is an update for bundle test-bundle1:
		 - Installed bundle last updated in version: 10
		 - Latest available version: 30
		Bundle size:
		 - Size of bundle: .* KB
		 - Size bundle takes on disk \(includes dependencies\): .* KB
	EOM
	)
	assert_regex_in_output "$expected_output"

}

@test "BIN002: Show info about a bundle not installed in the system" {

	# users can use the bundle-info command to see detailed info about a bundle
	# if the bundle is not installed it should show the latest available version
	# and how much disk space it would take if installed
	# also bundles that are experimental are marked as such in the status

	run sudo sh -c "$SWUPD bundle-info $SWUPD_OPTS test-bundle2"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_______________________________
		 Info for bundle: test-bundle2
		_______________________________
		Status: Not installed \(experimental\)
		Latest available version: 10
		Bundle size:
		 - Size of bundle: .* KB
		 - Maximum amount of disk size the bundle will take if installed \(includes dependencies\): .* KB
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "BIN003: Show info about a bundle for a particular system version" {

	# a specific version can be specified to display info

	run sudo sh -c "$SWUPD bundle-info $SWUPD_OPTS test-bundle1 --version 20"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_______________________________
		 Info for bundle: test-bundle1
		_______________________________
		Status: Installed
		There is an update for bundle test-bundle1:
		 - Installed bundle last updated in version: 20
		 - Latest available version: 30
		Bundle size:
		 - Size of bundle: .* KB
		 - Size bundle takes on disk \(includes dependencies\): .* KB
	EOM
	)
	assert_regex_in_output "$expected_output"

}

@test "BIN004: Show info about a bundle for a particular system version" {

	# version can take latest or current

	run sudo sh -c "$SWUPD bundle-info $SWUPD_OPTS test-bundle1 --version latest"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_______________________________
		 Info for bundle: test-bundle1
		_______________________________
		Status: Installed
		Bundle test-bundle1 is up to date:
		 - Installed bundle last updated in version: 30
		 - Latest available version: 30
		Bundle size:
		 - Size of bundle: .* KB
		 - Size bundle takes on disk \(includes dependencies\): .* KB
	EOM
	)
	assert_regex_in_output "$expected_output"

}

@test "BIN005: Try to show info about an invalid bundle" {

	run sudo sh -c "$SWUPD bundle-info $SWUPD_OPTS bad-bundle"

	assert_status_is "$SWUPD_INVALID_BUNDLE"
	expected_output=$(cat <<-EOM
		Error: Bundle "bad-bundle" is invalid
	EOM
	)
	assert_is_output "$expected_output"

}
