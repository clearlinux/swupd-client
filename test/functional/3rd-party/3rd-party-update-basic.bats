#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"

	add_third_party_repo "$TEST_NAME" 10 staging test-repo1
	add_third_party_repo "$TEST_NAME" 10 staging test-repo2

	create_bundle -L -t -n test-bundle1 -f /foo/file_1 -u test-repo1 "$TEST_NAME"
	create_bundle       -n test-bundle2 -f /bar/file_2 -u test-repo1 "$TEST_NAME"
	create_bundle -L    -n test-bundle3 -f /baz/file_3 -u test-repo2 "$TEST_NAME"

	create_version -p "$TEST_NAME" 20 10 staging test-repo1
	update_bundle "$TEST_NAME" test-bundle1 --update /foo/file_1 test-repo1
	update_bundle "$TEST_NAME" test-bundle1 --add /new_file test-repo1

}


@test "TPR036: Update 3rd-party bundles from a specific repository" {

	# users should be able to update specific 3rd-party repos to the latest version

	run sudo sh -c "$SWUPD 3rd-party update $SWUPD_OPTS --repo test-repo1"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Updates from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Update started
		Preparing to update from 10 to 20
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 1
		    new files         : 1
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Validating 3rd-party bundle file permissions...
		Installing files...
		Update was applied
		Updating 3rd-party bundle binaries...
		Warning: post-update helper scripts skipped due to --no-scripts argument
		Update successful - System updated from version 10 to version 20
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo1/new_file

}

@test "TPR037: Users can check if the 3rd-party bundles from a specifi repo are up to date" {

	run sudo sh -c "$SWUPD 3rd-party update $SWUPD_OPTS --repo test-repo1 --status"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Current OS version: 10
		Latest server version: 20
		There is a new OS version available: 20
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR038: Update 3rd-party bundles from all repositories" {

	# if no repo is specified, all 3rd-party repos should be updated

	run sudo sh -c "$SWUPD 3rd-party update $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		____________________________
		 3rd-Party Repo: test-repo1
		____________________________
		Updates from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Update started
		Preparing to update from 10 to 20
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 1
		    new files         : 1
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Validating 3rd-party bundle file permissions...
		Installing files...
		Update was applied
		Updating 3rd-party bundle binaries...
		Warning: post-update helper scripts skipped due to --no-scripts argument
		Update successful - System updated from version 10 to version 20
		____________________________
		 3rd-Party Repo: test-repo2
		____________________________
		Updates from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Update started
		Version on server (10) is not newer than system version (10)
		Update complete - System already up-to-date at version 10
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo1/new_file

}

@test "TPR039: Users can check if the 3rd-party bundles from all repos are up to date" {

	run sudo sh -c "$SWUPD 3rd-party update $SWUPD_OPTS --status"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		____________________________
		 3rd-Party Repo: test-repo1
		____________________________
		Current OS version: 10
		Latest server version: 20
		There is a new OS version available: 20
		____________________________
		 3rd-Party Repo: test-repo2
		____________________________
		Current OS version: 10
		Latest server version: 10
		There are no updates available
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=32
