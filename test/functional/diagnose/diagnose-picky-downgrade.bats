#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /file_1 "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	create_bundle -L -n test-bundle2 -f /usr/foo/file_2,/usr/foo/file_3,/bar/file_4,/bar/file_5 "$TEST_NAME"
	set_current_version "$TEST_NAME" 20

}

@test "DIA011: Diagnose can show files that would be removed if not available in a previous version" {

	run sudo sh -c "$SWUPD diagnose --picky --version=10 --force $SWUPD_OPTS"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Verifying version 10
		Warning: Bundle "test-bundle2" is invalid, skipping it...
		Warning: One or more installed bundles are not available at version 10.
		Generating list of extra files under .*/target-dir/usr
		/usr/share/defaults/swupd/versionurl
		/usr/share/defaults/swupd/contenturl
		/usr/share/clear/bundles/test-bundle2
		/usr/foo/file_3
		/usr/foo/file_2
		/usr/foo/
		Inspected 19 files
		  6 files found which should be deleted
		Diagnose successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/foo/file_2
	assert_file_exists "$TARGETDIR"/usr/foo/file_3
	assert_file_exists "$TARGETDIR"/bar/file_4
	assert_file_exists "$TARGETDIR"/bar/file_5
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/usr/share/defaults/swupd/versionurl
	assert_file_exists "$TARGETDIR"/usr/share/defaults/swupd/contenturl

}
