#!/usr/bin/env bats

# Author: William Douglas
# Email: william.douglas@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME" 10 1
	create_version -r "$TEST_NAME" 20 10 1
	create_version -r "$TEST_NAME" 30 20 1

}

test_teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "UPD077: Run update with --incremental old latest file format" {

	# Run update with --incremental where the latest file format only
        # contains a single line of 30

	run sudo sh -c "$SWUPD update $SWUPD_OPTS_NO_FMT --incremental"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 30
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Statistics for going from version 10 to version 30:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 2
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 30
	EOM
	)
	assert_is_output "$expected_output"

}

@test "UPD078: Run update with --incremental multiline latest file format" {

	# Run update with --incremental where the latest file format contains
	# "30\n20\n10"
	write_to_protected_file -a "$WEB_DIR/version/format1/latest" "\n20"
	write_to_protected_file -a "$WEB_DIR/version/format1/latest" "\n10"
	sign_version "$WEB_DIR/version/format1/latest"

	run sudo sh -c "$SWUPD update $SWUPD_OPTS_NO_FMT --incremental"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 20
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 2
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 20
		Update started
		Preparing to update from 20 to 30
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Statistics for going from version 20 to version 30:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 2
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 20 to version 30
	EOM
	)
	assert_is_output "$expected_output"

}

@test "UPD079: Run update with --incremental multiline latest missing current version" {

	# Run update with --incremental where the latest file format contains
	# "30\n20"
        # (the starting version is missing)
        # Test may be reasonable to delete but for now verify process robustness
	write_to_protected_file -a "$WEB_DIR/version/format1/latest" "\n20"
	sign_version "$WEB_DIR/version/format1/latest"

	run sudo sh -c "$SWUPD update $SWUPD_OPTS_NO_FMT --incremental"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 20
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 2
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 20
		Update started
		Preparing to update from 20 to 30
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Statistics for going from version 20 to version 30:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 2
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 20 to version 30
	EOM
	)
	assert_is_output "$expected_output"

}

@test "UPD080: Run update with --incremental multiline latest with target version" {

	# Run update with --incremental where the latest file format contains
	# "40\n30\n20\n10"
        # And a target version of 30
	create_version -r "$TEST_NAME" 40 30 1
	write_to_protected_file -a "$WEB_DIR/version/format1/latest" "\n30"
	write_to_protected_file -a "$WEB_DIR/version/format1/latest" "\n20"
	write_to_protected_file -a "$WEB_DIR/version/format1/latest" "\n10"
	sign_version "$WEB_DIR/version/format1/latest"

	run sudo sh -c "$SWUPD update $SWUPD_OPTS_NO_FMT --incremental -V 30"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 20
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 2
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 20
		Update started
		Preparing to update from 20 to 30
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Statistics for going from version 20 to version 30:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 2
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 20 to version 30
	EOM
	)
	assert_is_output "$expected_output"

}

@test "UPD081: Run update with normally but with a multiline target version" {

	# Run update without --incremental where the latest file format contains
	# "50\n40\n30\n20\n10"
        # 50 is chosen because at the time the test was written handling
        # of a latest file with "50\n40\n30\n20\n10" was failing case due to
        # file size but it should not fail going forward.
	create_version -r "$TEST_NAME" 40 30 1
	create_version -r "$TEST_NAME" 50 40 1
	write_to_protected_file -a "$WEB_DIR/version/format1/latest" "\n40"
	write_to_protected_file -a "$WEB_DIR/version/format1/latest" "\n30"
	write_to_protected_file -a "$WEB_DIR/version/format1/latest" "\n20"
	write_to_protected_file -a "$WEB_DIR/version/format1/latest" "\n10"
	sign_version "$WEB_DIR/version/format1/latest"

	run sudo sh -c "$SWUPD update $SWUPD_OPTS_NO_FMT"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 50
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Statistics for going from version 10 to version 50:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 2
		    new files         : 0
		    deleted files     : 0
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Update was applied
		Calling post-update helper scripts
		Update successful - System updated from version 10 to version 50
	EOM
	)
	assert_is_output "$expected_output"

}
