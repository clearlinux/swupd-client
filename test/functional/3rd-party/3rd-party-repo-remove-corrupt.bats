#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

metadata_setup() {

	create_test_environment "$TEST_NAME"
	create_third_party_repo -a "$TEST_NAME" 10 1 repo1
	create_bundle -L -t -n test-bundle1 -f /file_1,/bin/binary_1 -u repo1 "$TEST_NAME"
	# delete some files to make the repo corrupt
	sudo rm "$PATH_PREFIX"/"$THIRD_PARTY_BUNDLES_DIR"/repo1/usr/lib/os-release

}

@test "TPR074: Attempt to remove a corrupt third party repository" {

	# user should still be able to remove corrupt 3rd-party repositories

	run sudo sh -c "$SWUPD 3rd-party remove $SWUPD_OPTS repo1"

	assert_status_is "$SWUPD_CURRENT_VERSION_UNKNOWN"
	expected_output=$(cat <<-EOM
		Error: Unable to determine current version for repository repo1
		Error: The 3rd-party repository repo1 is corrupt and cannot be removed cleanly
		To force the removal of the repository use the --force option
		Please be aware that this may leave exported files in $PATH_PREFIX/$THIRD_PARTY_BIN_DIR
		Failed to remove repository
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR075: Force the removal of a corrupt third party repository" {

	# user should still be able to remove corrupt 3rd-party repositories

	run sudo sh -c "$SWUPD 3rd-party remove $SWUPD_OPTS repo1 --force"

	assert_status_is "$SWUPD_CURRENT_VERSION_UNKNOWN"
	expected_output=$(cat <<-EOM
		Error: Unable to determine current version for repository repo1
		Warning: The --force option is specified; forcing the removal of the repository
		Please be aware that this may leave exported files in $PATH_PREFIX/$THIRD_PARTY_BIN_DIR
		Removing repository repo1...
		Repository and its content partially removed
	EOM
	)
	assert_is_output "$expected_output"

}
