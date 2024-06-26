#!/usr/bin/env bats

load "../testlib"

setup_file() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -t -n test-bundle1 -d /foo -f /test-file1,/bar/test-file2,/bat/test-file3,/bat/common "$TEST_NAME"
	create_bundle -L -t -n test-bundle2 -f /bat/test-file4,/bat/common "$TEST_NAME"
	create_bundle -n test-bundle3 -f /baz/test-file5 "$TEST_NAME"

}

teardown_file() {

	destroy_test_environment --force "$TEST_NAME"

}

test_teardown() {

	# reinstall test-bundle1 and test-bundle2
	install_bundle "$WEB_DIR"/10/Manifest.test-bundle1
	install_bundle "$WEB_DIR"/10/Manifest.test-bundle2

}

# ------------------------------------------
# Good Cases (all good bundles)
# ------------------------------------------

@test "REM001: Removing one bundle" {

	sudo mkdir "$TARGET_DIR"/bar/keep1
	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1"

	assert_status_is 0
	# bundle1 was removed
	assert_file_not_exists "$TARGET_DIR"/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$TARGET_DIR"/test-file1
	assert_file_not_exists "$TARGET_DIR"/bar/test-file2
	assert_file_not_exists "$TARGET_DIR"/bat/test-file3
	assert_dir_not_exists "$TARGET_DIR"/foo
	assert_dir_not_exists "$TARGET_DIR"/bar
	assert_file_not_exists "$STATE_DIR"/bundles/test-bundle1
        # keep file isn't removed
	assert_file_exists "$TARGET_DIR"/.deleted.*.bar/keep1
	# bundle2 was not removed
	assert_file_exists "$TARGET_DIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGET_DIR"/bat/test-file4
	assert_file_exists "$TARGET_DIR"/bat/common
	assert_file_exists "$STATE_DIR"/bundles/test-bundle2
	expected_output=$(cat <<-EOM
		The following bundles are being removed:
		 - test-bundle1
		Deleting bundle files...
		Total deleted files: 6
		Successfully removed 1 bundle
	EOM
	)

}

@test "REM002: Removing multiple bundles" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1 test-bundle2"

	assert_status_is 0
	# bundle1 and 2 were removed
	assert_file_not_exists "$TARGET_DIR"/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$TARGET_DIR"/usr/share/clear/bundles/test-bundle2
	assert_file_not_exists "$TARGET_DIR"/test-file1
	assert_file_not_exists "$TARGET_DIR"/bar/test-file2
	assert_file_not_exists "$TARGET_DIR"/bat/test-file3
	assert_file_not_exists "$TARGET_DIR"/bat/test-file4
	assert_file_not_exists "$TARGET_DIR"/bat/common
	assert_dir_not_exists "$TARGET_DIR"/foo
	assert_dir_not_exists "$TARGET_DIR"/bar
	assert_dir_not_exists "$TARGET_DIR"/bat
	assert_file_not_exists "$STATE_DIR"/bundles/test-bundle1
	assert_file_not_exists "$STATE_DIR"/bundles/test-bundle2
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
	assert_file_not_exists "$TARGET_DIR"/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$TARGET_DIR"/test-file1
	assert_file_not_exists "$TARGET_DIR"/bar/test-file2
	assert_file_not_exists "$TARGET_DIR"/bat/test-file3
	assert_dir_not_exists "$TARGET_DIR"/foo
	assert_dir_not_exists "$TARGET_DIR"/bar
	# bundle2 was not removed
	assert_file_exists "$TARGET_DIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGET_DIR"/bat/test-file4
	assert_file_exists "$TARGET_DIR"/bat/common
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
	assert_file_not_exists "$TARGET_DIR"/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$TARGET_DIR"/test-file1
	assert_file_not_exists "$TARGET_DIR"/bar/test-file2
	assert_file_not_exists "$TARGET_DIR"/bat/test-file3
	assert_dir_not_exists "$TARGET_DIR"/foo
	assert_dir_not_exists "$TARGET_DIR"/bar
	# bundle2 was not removed
	assert_file_exists "$TARGET_DIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$TARGET_DIR"/bat/test-file4
	assert_file_exists "$TARGET_DIR"/bat/common
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
#WEIGHT=6
