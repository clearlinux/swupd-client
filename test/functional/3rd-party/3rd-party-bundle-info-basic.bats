#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle-upstream -f /file_upstream "$TEST_NAME"

	# create a couple of 3rd-party repos with bundles
	add_third_party_repo "$TEST_NAME" 10 staging repo1
	create_bundle    -n test-bundle1 -f /foo/file_1A -u repo1 "$TEST_NAME"
	create_bundle -L -n test-bundle2 -f /bar/file_2  -u repo1 "$TEST_NAME"
	add_dependency_to_manifest "$TEST_NAME"/3rd-party/repo1/10/Manifest.test-bundle2 test-bundle1

	add_third_party_repo "$TEST_NAME" 10 staging repo2
	create_bundle    -n test-bundle1 -f /baz/file_1B -u repo2 "$TEST_NAME"
	create_bundle    -n test-bundle3 -f /baz/file_3  -u repo2 "$TEST_NAME"
	# when swupd initializes, if the tracking directory is empty, it will
	# mark all installed bundles as tracked, to avoid this, let's add a dummy
	# value to the tracking directory
	sudo touch "$STATEDIR"/3rd-party/{repo1,repo2}/bundles/dummy

}

test_setup() {

	return

}

test_teardown() {

	return
}

global_teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "TPR031: Show info about a 3rd-party bundle installed in the system" {

	run sudo sh -c "$SWUPD 3rd-party bundle-info $SWUPD_OPTS test-bundle2"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle2 in the 3rd-party repositories...
		Bundle test-bundle2 found in 3rd-party repository repo1
		_______________________________
		 Info for bundle: test-bundle2
		_______________________________
		Status: Installed
		Bundle test-bundle2 is up to date:
		 - Installed bundle last updated in version: 10
		 - Latest available version: 10
		Bundle size:
		 - Size of bundle: .* KB
		 - Size bundle takes on disk \(includes dependencies\): .* KB
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "TPR032: Show info about a 3rd-party bundle not installed in the system" {

	run sudo sh -c "$SWUPD 3rd-party bundle-info $SWUPD_OPTS test-bundle3"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle3 in the 3rd-party repositories...
		Bundle test-bundle3 found in 3rd-party repository repo2
		_______________________________
		 Info for bundle: test-bundle3
		_______________________________
		Status: Not installed
		Latest available version: 10
		Bundle size:
		 - Size of bundle: .* KB
		 - Maximum amount of disk size the bundle will take if installed \(includes dependencies\): .* KB
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "TPR033: Show info about a 3rd-party bundle displaying its dependencies and files" {

	run sudo sh -c "$SWUPD 3rd-party bundle-info $SWUPD_OPTS test-bundle2 --files --dependencies"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle2 in the 3rd-party repositories...
		Bundle test-bundle2 found in 3rd-party repository repo1
		_______________________________
		 Info for bundle: test-bundle2
		_______________________________
		Status: Installed
		Bundle test-bundle2 is up to date:
		 - Installed bundle last updated in version: 10
		 - Latest available version: 10
		Bundle size:
		 - Size of bundle: .* KB
		 - Size bundle takes on disk \(includes dependencies\): .* KB
		Direct dependencies \(1\):
		 - test-bundle1
		Files in bundle \(includes dependencies\):
		 - /bar
		 - /bar/file_2
		 - /foo
		 - /foo/file_1A
		 - /usr
		 - /usr/share
		 - /usr/share/clear
		 - /usr/share/clear/bundles
		 - /usr/share/clear/bundles/test-bundle1
		 - /usr/share/clear/bundles/test-bundle2
		Total files: 10
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "TPR034: Try showing info about a 3rd-party bundle that exists in many third party repos" {

	run sudo sh -c "$SWUPD 3rd-party bundle-info $SWUPD_OPTS test-bundle1"

	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle1 in the 3rd-party repositories...
		Bundle test-bundle1 found in 3rd-party repository repo1
		Bundle test-bundle1 found in 3rd-party repository repo2
		Error: bundle test-bundle1 was found in more than one 3rd-party repository
		Please specify a repository using the --repo flag
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR035: Show info about a 3rd-party bundle from a specific repo" {

	run sudo sh -c "$SWUPD 3rd-party bundle-info $SWUPD_OPTS test-bundle1 --repo repo2"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_______________________________
		 Info for bundle: test-bundle1
		_______________________________
		Status: Not installed
		Latest available version: 10
		Bundle size:
		 - Size of bundle: .* KB
		 - Maximum amount of disk size the bundle will take if installed \(includes dependencies\): .* KB
	EOM
	)
	assert_regex_is_output "$expected_output"

}
