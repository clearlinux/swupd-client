#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME"
	create_bundle -L -n test-bundle -f /usr/lib/kernel/test-file "$TEST_NAME"

}

@test "REM008: Removing a bundle containing a boot file" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle"

	assert_status_is 0
	assert_file_not_exists "$TARGET_DIR"/usr/share/clear/bundles/test-bundle
	assert_file_not_exists "$TARGET_DIR"/usr/lib/kernel/testfile
	assert_dir_not_exists "$TARGET_DIR"/usr/lib/kernel
	# these files should not be deleted because they are also part of os-core
	assert_dir_exists "$TARGET_DIR"/usr/lib
	assert_dir_exists "$TARGET_DIR"/usr/
	expected_output=$(cat <<-EOM
		The following bundles are being removed:
		 - test-bundle
		Deleting bundle files...
		Total deleted files: 3
		Successfully removed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=3
