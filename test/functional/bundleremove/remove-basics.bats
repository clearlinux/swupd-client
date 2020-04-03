#!/usr/bin/env bats

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -t -n test-bundle1 -d /foo -f /test-file1,/bar/test-file2,/bat/test-file3,/bat/common "$TEST_NAME"
	create_bundle -L -t -n test-bundle2 -f /bat/test-file4,/bat/common "$TEST_NAME"
	create_bundle -n test-bundle3 -f /baz/test-file5 "$TEST_NAME"

}

test_teardown() {

	# reinstall test-bundle1 and test-bundle2
	install_bundle "$WEBDIR"/10/Manifest.test-bundle1
	install_bundle "$WEBDIR"/10/Manifest.test-bundle2

}

global_teardown() {

	destroy_test_environment "$TEST_NAME"

}

# ------------------------------------------
# Good Cases (all good bundles)
# ------------------------------------------

@test "REM001: Removing one bundle" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1"

	assert_status_is 0
	# bundle1 was removed
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$TARGETDIR"/test-file1
	assert_file_not_exists "$TARGETDIR"/bar/test-file2
	assert_file_not_exists "$TARGETDIR"/bat/test-file3
	assert_dir_not_exists "$TARGETDIR"/foo
	assert_dir_not_exists "$TARGETDIR"/bar
	assert_file_not_exists "$STATEDIR"/bundles/test-bundle1
	# bundle2 was not removed
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/bat/test-file4
	assert_file_exists "$TARGETDIR"/bat/common
	assert_file_exists "$STATEDIR"/bundles/test-bundle2
	expected_output=$(cat <<-EOM
		The following bundles are being removed:
		 - test-bundle1
		Deleting bundle files...
		Total deleted files: 6
		Successfully removed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

}

@test "REM002: Removing multiple bundles" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1 test-bundle2"

	assert_status_is 0
	# bundle1 and 2 were removed
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_not_exists "$TARGETDIR"/test-file1
	assert_file_not_exists "$TARGETDIR"/bar/test-file2
	assert_file_not_exists "$TARGETDIR"/bat/test-file3
	assert_file_not_exists "$TARGETDIR"/bat/test-file4
	assert_file_not_exists "$TARGETDIR"/bat/common
	assert_dir_not_exists "$TARGETDIR"/foo
	assert_dir_not_exists "$TARGETDIR"/bar
	assert_dir_not_exists "$TARGETDIR"/bat
	assert_file_not_exists "$STATEDIR"/bundles/test-bundle1
	assert_file_not_exists "$STATEDIR"/bundles/test-bundle2
	expected_output=$(cat <<-EOM
		The following bundles are being removed:
		 - test-bundle2
		 - test-bundle1
		Deleting bundle files...
		Total deleted files: 9
		Successfully removed 2 bundles
	EOM
	)
	assert_is_output "$expected_output"

}

# ------------------------------------------
# Bad Cases (all bad bundles)
# ------------------------------------------

@test "REM003: Try removing a bundle that is not installed" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle3"

	assert_status_is "$SWUPD_BUNDLE_NOT_TRACKED"
	expected_output=$(cat <<-EOM

		Warning: Bundle "test-bundle3" is not installed, skipping it...

		Failed to remove 1 of 1 bundles
	EOM
	)
	assert_is_output --identical "$expected_output"

}

@test "REM004: Try removing a bundle that does not exist" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS fake-bundle"

	assert_status_is "$SWUPD_INVALID_BUNDLE"
	expected_output=$(cat <<-EOM

		Warning: Bundle "fake-bundle" is invalid, skipping it...

		Failed to remove 1 of 1 bundles
	EOM
	)
	assert_is_output --identical "$expected_output"

}

@test "REM005: Try removing multiple bundles, all invalid, one non existent, one already removed" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS fake-bundle test-bundle3"

	assert_status_is "$SWUPD_BUNDLE_NOT_TRACKED"
	expected_output=$(cat <<-EOM

		Warning: Bundle "fake-bundle" is invalid, skipping it...

		Warning: Bundle "test-bundle3" is not installed, skipping it...

		Failed to remove 2 of 2 bundles
	EOM
	)
	assert_is_output --identical "$expected_output"

}

# ------------------------------------------
# Partial Cases (at least one good bundle)
# ------------------------------------------

@test "REM006: Try removing multiple bundles, one valid, one not installed" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle3 test-bundle1"

	assert_status_is "$SWUPD_BUNDLE_NOT_TRACKED"
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$TARGETDIR"/test-file1
	assert_file_not_exists "$TARGETDIR"/bar/test-file2
	assert_file_not_exists "$TARGETDIR"/bat/test-file3
	assert_dir_not_exists "$TARGETDIR"/foo
	assert_dir_not_exists "$TARGETDIR"/bar
	# bundle2 was not removed
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/bat/test-file4
	assert_file_exists "$TARGETDIR"/bat/common
	expected_output=$(cat <<-EOM
		Warning: Bundle "test-bundle3" is not installed, skipping it...
		The following bundles are being removed:
		 - test-bundle1
		Deleting bundle files...
		Total deleted files: 6
		Failed to remove 1 of 2 bundles
	EOM
	)
	assert_is_output "$expected_output"

}

@test "REM007: Try removing multiple bundles, one valid, one non existent" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS fake-bundle test-bundle1"

	assert_status_is "$SWUPD_INVALID_BUNDLE"
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$TARGETDIR"/test-file1
	assert_file_not_exists "$TARGETDIR"/bar/test-file2
	assert_file_not_exists "$TARGETDIR"/bat/test-file3
	assert_dir_not_exists "$TARGETDIR"/foo
	assert_dir_not_exists "$TARGETDIR"/bar
	# bundle2 was not removed
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/bat/test-file4
	assert_file_exists "$TARGETDIR"/bat/common
	expected_output=$(cat <<-EOM
		Warning: Bundle "fake-bundle" is invalid, skipping it...
		The following bundles are being removed:
		 - test-bundle1
		Deleting bundle files...
		Total deleted files: 6
		Failed to remove 1 of 2 bundles
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=5
