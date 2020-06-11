#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME" 10 1
	create_third_party_repo -a "$TEST_NAME" 10 1 repo1
	WEB_DIR="$TP_WEB_DIR"
	create_third_party_repo -a "$TEST_NAME" 10 1 repo2
	create_bundle -L -t -n test-bundle1 -f /file_1 -u repo1 "$TEST_NAME"
	create_bundle -L    -n test-bundle2 -f /file_2 -u repo1 "$TEST_NAME"
	create_bundle -L    -n test-bundle3 -f /file_3 -u repo1 "$TEST_NAME"
	create_bundle -L    -n test-bundle4 -f /file_4 -u repo1 "$TEST_NAME"
	create_bundle -L    -n test-bundle5 -f /file_5 -u repo2 "$TEST_NAME"
	add_dependency_to_manifest "$WEB_DIR"/10/Manifest.test-bundle1 test-bundle2
	add_dependency_to_manifest "$WEB_DIR"/10/Manifest.test-bundle3 test-bundle4

}

@test "TPR095: Remove orphaned bundles from 3rd-party repositories" {

	run sudo sh -c "$SWUPD 3rd-party bundle-remove $SWUPD_OPTS --orphans"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		_____________________________
		 3rd-Party Repository: repo1
		_____________________________
		The following bundles are being removed:
		 - test-bundle4
		 - test-bundle3
		Deleting bundle files...
		Total deleted files: 4
		Removing 3rd-party bundle binaries...
		Successfully removed 2 bundles
		_____________________________
		 3rd-Party Repository: repo2
		_____________________________
		Removing 3rd-party bundle binaries...
		No orphaned bundles found, nothing was removed
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR096: Remove orphaned bundles from 3rd-party repositories using --repo" {

	run sudo sh -c "$SWUPD 3rd-party bundle-remove $SWUPD_OPTS --orphans --repo repo1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		The following bundles are being removed:
		 - test-bundle4
		 - test-bundle3
		Deleting bundle files...
		Total deleted files: 4
		Removing 3rd-party bundle binaries...
		Successfully removed 2 bundles
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR097: Try removing orphaned bundles using invalid options" {

	run sudo sh -c "$SWUPD 3rd-party bundle-remove $SWUPD_OPTS --orphans --force"

	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: --orphans and --force options are mutually exclusive
	EOM
	)
	assert_in_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party bundle-remove $SWUPD_OPTS --orphans --recursive"

	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: --orphans and --recursive options are mutually exclusive
	EOM
	)
	assert_in_output "$expected_output"

	run sudo sh -c "$SWUPD 3rd-party bundle-remove $SWUPD_OPTS --orphans test-bundle1"

	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: you cannot specify bundles to remove when using --orphans
	EOM
	)
	assert_in_output "$expected_output"

}
