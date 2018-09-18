#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME"
	create_bundle -L -n os-core -f /usr/lib/kernel/testfile "$TEST_NAME"
	update_manifest "$WEBDIR"/10/Manifest.os-core file-status /usr/lib/kernel/testfile .db.
	update_manifest "$WEBDIR"/10/Manifest.os-core file-hash /usr/lib/kernel/testfile "$zero_hash"

}

@test "verify override boot update" {

	run sudo sh -c "$SWUPD verify --fix -b $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 10
		Verifying files
		Adding any missing files
		Fixing modified files
		Inspected 4 files
		  0 files were missing
		  0 files found which should be deleted
		Calling post-update helper scripts.
		WARNING: boot files update skipped due to --no-boot-update argument
		Fix successful
	EOM
	)
	assert_is_output "$expected_output"
	# this should exist at the end, even if cbm is not run
	assert_file_exists "$TARGETDIR"/usr/lib/kernel/testfile

}