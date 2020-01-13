#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"

	add_third_party_repo "$TEST_NAME" 10 staging test-repo1
	create_bundle -L -t -n test-bundle1 -f /foo/file_1 -u test-repo1 "$TEST_NAME"
	create_version -p "$TEST_NAME" 20 10 staging test-repo1
	update_bundle "$TEST_NAME" test-bundle1 --update /foo/file_1 test-repo1
	create_version -p "$TEST_NAME" 30 20 staging test-repo1
	update_bundle "$TEST_NAME" test-bundle1 --add /new_file test-repo1

}

@test "TPR040: Update 3rd-party bundles to a specific version" {

	# users should be able to specify the version to which the update should happen

	run sudo sh -c "$SWUPD 3rd-party update $SWUPD_OPTS --repo test-repo1 --version 20"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
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
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 20
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo1/new_file

}

@test "TPR041: Try updating 3rd-party bundles to a specific version in all repos" {

	# a user needs to specify a repo in order to use the --version flag

	run sudo sh -c "$SWUPD 3rd-party update $SWUPD_OPTS --version 20"

	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: a repository needs to be specified to use the --version flag
	EOM
	)
	assert_in_output "$expected_output"

}
