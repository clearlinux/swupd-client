#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n upstream-bundle -f /upstream_file "$TEST_NAME"

	# create a couple 3rd-party repos within the test environment and add
	# some bundles to them
	add_third_party_repo "$TEST_NAME" 10 1 test-repo1
	create_bundle -L -n test-bundle1 -f /foo/file_1 -u test-repo1 "$TEST_NAME"

	add_third_party_repo "$TEST_NAME" 10 1 test-repo2
	create_bundle -L -t -n test-bundle1 -f /foo/file_2 -u test-repo2 "$TEST_NAME"
	create_bundle -L    -n test-bundle2 -f /bar/file_3 -u test-repo2 "$TEST_NAME"
	add_dependency_to_manifest "$TEST_NAME"/3rd-party/test-repo2/10/Manifest.test-bundle1 test-bundle2

}

@test "TPR027: Try removing a bundle that exists in many third party repos" {

	run sudo sh -c "$SWUPD 3rd-party bundle-remove $SWUPD_OPTS test-bundle1"

	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle1 in the 3rd-party repositories...
		Bundle test-bundle1 found in 3rd-party repository test-repo1
		Bundle test-bundle1 found in 3rd-party repository test-repo2
		Error: bundle test-bundle1 was found in more than one 3rd-party repository
		Please specify a repository using the --repo flag
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR028: Removing a bundle that exists in many third party repos specifying the repo" {

	run sudo sh -c "$SWUPD 3rd-party bundle-remove $SWUPD_OPTS test-bundle1 --repo test-repo2"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		The following bundles are being removed:
		 - test-bundle1
		Deleting bundle files...
		Total deleted files: 3
		Successfully removed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR029: Removing a bundle and its dependencies specifying the repo" {

	run sudo sh -c "$SWUPD 3rd-party bundle-remove $SWUPD_OPTS test-bundle1 --repo test-repo2 --recursive"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: The --recursive option was used, the specified bundle and its dependencies will be removed from the system
		The following bundles are being removed:
		 - test-bundle1
		 - test-bundle2
		Deleting bundle files...
		Total deleted files: 6
		Successfully removed 1 bundle
		1 bundle that was installed as a dependency was removed
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR030: Removing a bundle and all bundles that depend on it specifying the repo" {

	run sudo sh -c "$SWUPD 3rd-party bundle-remove $SWUPD_OPTS test-bundle2 --repo test-repo2 --force"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: The --force option was used, the specified bundle and all bundles that require it will be removed from the system
		Bundle "test-bundle2" is required by the following bundles:
		 - test-bundle1
		The following bundles are being removed:
		 - test-bundle2
		 - test-bundle1
		Deleting bundle files...
		Total deleted files: 6
		Successfully removed 1 bundle
		1 bundle that depended on the specified bundle(s) was removed
	EOM
	)
	assert_is_output "$expected_output"

}
