#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_third_party_repo "$TEST_NAME" 10 staging repo1
	# create an empty content directory that matches the repo name
	sudo mkdir -p "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/repo1

}

@test "TPR078: Try adding a 3rd-party repo that has an empty content directory conflict" {

	# when adding a 3rd-party repo, if a directory with the same name already exists
	# and is not empty, swupd should warn the user and abort unless the --force option is used
	# if the directory exists, but is empty, the process should continue business as usual

	run sudo sh -c "$SWUPD 3rd-party add repo1 file://$TPURL $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Adding 3rd-party repository repo1...
		Installing the required bundle 'os-core' from the repository...
		Note that all bundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Loading required manifests...
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Warning: post-update helper scripts skipped due to --no-scripts argument
		Successfully installed 1 bundle
		Repository added successfully
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR079: Try adding a 3rd-party repo that has a content directory conflict" {

	# when adding a 3rd-party repo, if a directory with the same name already exists
	# and is not empty, swupd should warn the user and abort unless the --force option is used
	# if the directory exists, but is empty, the process should continue business as usual

	sudo touch "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/repo1/leftover

	run sudo sh -c "$SWUPD 3rd-party add repo1 file://$TPURL $SWUPD_OPTS"

	assert_status_is "$SWUPD_INVALID_REPOSITORY"
	expected_output=$(cat <<-EOM
		Error: A content directory for a 3rd-party repository called "repo1" already exists at $PATH_PREFIX/opt/3rd-party/bundles/repo1, aborting...
		To force the removal of the directory and continue adding the repository use the --force option
		Failed to add repository
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR080: Force adding a 3rd-party repo that has a content directory conflict" {

	# when adding a 3rd-party repo, if a directory with the same name already exists
	# and is not empty, swupd should warn the user and abort unless the --force option is used
	# if the directory exists, but is empty, the process should continue business as usual

	sudo touch "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/repo1/leftover

	run sudo sh -c "$SWUPD 3rd-party add repo1 file://$TPURL $SWUPD_OPTS --force"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: A content directory for a 3rd-party repository called "repo1" already exists at $PATH_PREFIX/opt/3rd-party/bundles/repo1
		The --force option was used; forcing the removal of the directory
		Adding 3rd-party repository repo1...
		Installing the required bundle 'os-core' from the repository...
		Note that all bundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Loading required manifests...
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Warning: post-update helper scripts skipped due to --no-scripts argument
		Successfully installed 1 bundle
		Repository added successfully
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=8
