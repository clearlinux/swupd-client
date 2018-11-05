#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME"
	create_bundle -L -n test-bundle -f /usr/foo -d/usr/share/clear/bundles "$TEST_NAME"
	update_manifest "$WEBDIR"/10/Manifest.test-bundle file-status /usr/foo .g..

}

@test "verify ghosted file skipped during --picky" {

	run sudo sh -c "$SWUPD verify --fix --picky $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 10
		Verifying files
		Adding any missing files
		Fixing modified files
		--picky removing extra files under .*/target-dir/usr
		Inspected 13 files
		Calling post-update helper scripts.
		Fix successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	# this should exist at the end, despite being marked as "ghosted" in the
	# Manifest. With the ghosted file existing under /usr this test is to make
	# sure the ghosted files aren't removed during the --picky flow.
	assert_file_exists "$TARGETDIR"/usr/foo

}
