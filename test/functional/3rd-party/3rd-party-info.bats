#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

metadata_setup() {

	create_test_environment "$TEST_NAME"

	create_third_party_repo -a "$TEST_NAME" 10 1 repo1
	create_bundle -n test-bundle1 -f /foo/file_1 -u repo1 "$TEST_NAME"

	create_third_party_repo -a "$TEST_NAME" 20 5 repo2
	create_bundle -n test-bundle2 -f /foo/file_2 -u repo2 "$TEST_NAME"

}

@test "TPR076: Check 3rd-party info verbose" {

	run sudo sh -c "$SWUPD 3rd-party info $SWUPD_OPTS --verbose"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_______________________
		 3rd-Party Repo: repo1
		_______________________
		Distribution:      Swupd Test Distro
		Installed version: 10 (format 1)
		Version URL:       file://$TP_BASE_DIR/repo1
		Content URL:       file://$TP_BASE_DIR/repo1
		_______________________
		 3rd-Party Repo: repo2
		_______________________
		Distribution:      Swupd Test Distro
		Installed version: 20 (format 5)
		Version URL:       file://$TP_BASE_DIR/repo2
		Content URL:       file://$TP_BASE_DIR/repo2
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR077: Check 3rd-party info basic for a specific repo" {

	run sudo sh -c "$SWUPD 3rd-party info $SWUPD_OPTS --repo repo2"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Distribution:      Swupd Test Distro
		Installed version: 20
		Version URL:       file://$TP_BASE_DIR/repo2
		Content URL:       file://$TP_BASE_DIR/repo2
	EOM
	)
	assert_is_output "$expected_output"

}
