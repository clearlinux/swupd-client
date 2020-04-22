#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"

	# create a couple of 3rd-party repos with bundles
	create_third_party_repo -a "$TEST_NAME" 10 staging test-repo1
	create_bundle -L -t -n test-bundle1 -f /foo/file_1 -u test-repo1 "$TEST_NAME"
	create_version -p "$TEST_NAME" 20 10 staging test-repo1
	update_bundle "$TEST_NAME" test-bundle1 --update /foo/file_1 test-repo1

	create_third_party_repo -a "$TEST_NAME" 10 staging test-repo2
	create_bundle -L -t -n test-bundle2 -f /bar/file_2 -u test-repo2 "$TEST_NAME"

}

@test "TPR053: Check for available updates on third party repos" {

	run sudo sh -c "$SWUPD 3rd-party check-update $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		__________________________________
		 3rd-Party Repository: test-repo1
		__________________________________

		Current OS version: 10
		Latest server version: 20
		There is a new OS version available: 20


		__________________________________
		 3rd-Party Repository: test-repo2
		__________________________________

		Current OS version: 10
		Latest server version: 10
		There are no updates available
	EOM
	)
	assert_is_output --identical "$expected_output"

}

@test "TPR054: Check for available updates on a specific third party repo that has updates" {

	run sudo sh -c "$SWUPD 3rd-party check-update $SWUPD_OPTS --repo test-repo1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Current OS version: 10
		Latest server version: 20
		There is a new OS version available: 20
	EOM
	)
	assert_is_output --identical "$expected_output"

}

@test "TPR055: Check for available updates on a specific third party repo that doesn't have updates" {

	run sudo sh -c "$SWUPD 3rd-party check-update $SWUPD_OPTS --repo test-repo2"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Current OS version: 10
		Latest server version: 10
		There are no updates available
	EOM
	)
	assert_is_output --identical "$expected_output"

}
#WEIGHT=8
