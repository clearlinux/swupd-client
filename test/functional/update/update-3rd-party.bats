#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"

	create_bundle -L -t -n upstream-bundle -f /upstream_file "$TEST_NAME"
	create_version -p "$TEST_NAME" 20 10 staging
	update_bundle "$TEST_NAME" upstream-bundle --update /upstream_file

	create_third_party_repo -a "$TEST_NAME" 10 staging repo1
	create_bundle -L -t -n test-bundle1 -f /foo/file_1 -u repo1 "$TEST_NAME"
	create_version -p "$TEST_NAME" 20 10 staging repo1
	update_bundle "$TEST_NAME" test-bundle1 --update /foo/file_1 repo1
	update_bundle "$TEST_NAME" test-bundle1 --add /new_file repo1

}

@test "UPD073: Updating a system along with all the 3rd-party content" {

	# If the --3rd-party flag is used, if the update completes successfully
	# the 3rd-party content is also updated

	run sudo sh -c "$SWUPD update $SWUPD_OPTS --3rd-party"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 20 (in format staging)
		Downloading packs for:
		 - upstream-bundle
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
		Updating content from 3rd-party repositories
		_____________________________
		 3rd-Party Repository: repo1
		_____________________________
		Updates from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Update started
		Preparing to update from 10 to 20 (in format staging)
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 2
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 2
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

}

@test "UPD074: Updating a system along with all the 3rd-party content using --json-output" {

	# If the --3rd-party flag is used, if the update completes successfully
	# the 3rd-party content is also updated

	run sudo sh -c "$SWUPD update $SWUPD_OPTS --3rd-party --json-output"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		[
		{ "type" : "start", "section" : "update" },
		{ "type" : "info", "msg" : "Update started" },
	EOM
	)
	assert_in_output "$expected_output"
	expected_output=$(cat <<-EOM
		{ "type" : "info", "msg" : "Update successful - System updated from version 10 to version 20" },
		{ "type" : "end", "section" : "update", "status" : 0 }
		]
		[
		{ "type" : "start", "section" : "3rd-party-update" },
		{ "type" : "info", "msg" : "_____________________________" },
		{ "type" : "info", "msg" : " 3rd-Party Repository: repo1" },
		{ "type" : "info", "msg" : "_____________________________" },
	EOM
	)
	assert_in_output "$expected_output"
	expected_output=$(cat <<-EOM
		{ "type" : "info", "msg" : "Update successful - System updated from version 10 to version 20" },
		{ "type" : "end", "section" : "3rd-party-update", "status" : 0 }
		]
	EOM
	)
	assert_in_output "$expected_output"

}

@test "UPD075: Checking update status along with all the 3rd-party content" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS --3rd-party --status"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Current OS version: 10
		Latest server version: 20
		There is a new OS version available: 20
		Checking update status of content from 3rd-party repositories
		_____________________________
		 3rd-Party Repository: repo1
		_____________________________
		Current OS version: 10
		Latest server version: 20
		There is a new OS version available: 20
	EOM
	)
	assert_is_output "$expected_output"

}

@test "UPD076: There are no undesired behaviors when using --3rd-party along with options not supported in 3rd-party" {

	# the --update-search-file-index flag is not supported with
	# "3rd-party update", so there should be no side effects when
	# using "update --3rd-party --update-search-file-index"

	run sudo sh -c "$SWUPD update $SWUPD_OPTS --3rd-party --update-search-file-index"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 20 (in format staging)
		Downloading packs for:
		 - upstream-bundle
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
		Downloading all Clear Linux manifests...
		Update successful - System updated from version 10 to version 20
		Updating content from 3rd-party repositories
		_____________________________
		 3rd-Party Repository: repo1
		_____________________________
		Updates from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons
		Update started
		Preparing to update from 10 to 20 (in format staging)
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 2
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 2
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

}
