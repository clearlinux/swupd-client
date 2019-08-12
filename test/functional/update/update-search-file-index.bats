#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo/test-file1 "$TEST_NAME"
	create_version -p "$TEST_NAME" 100 10
	update_bundle "$TEST_NAME" os-core --add /foo

}

@test "UPD059: Download search-file index on update" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS --update-search-file-index"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 100
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Statistics for going from version 10 to version 100:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 0
		    new files         : 1
		    deleted files     : 0
		No extra files need to be downloaded
		Staging file content
		Applying update
		Update was applied
		Calling post-update helper scripts
		Downloading all Clear Linux manifests
		Update successful - System updated from version 10 to version 100
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$STATEDIR"/10/Manifest.test-bundle1
}

@test "UPD060: Don't download search-file index on update" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 100
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Statistics for going from version 10 to version 100:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 0
		    new files         : 1
		    deleted files     : 0
		No extra files need to be downloaded
		Staging file content
		Applying update
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 100
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_not_exists "$STATEDIR"/10/Manifest.test-bundle1
}

