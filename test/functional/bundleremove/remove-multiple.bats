#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /foo/file1 "$TEST_NAME"
	create_bundle -L -n test-bundle2 -f /bar/file2 "$TEST_NAME"
	create_bundle -L -n test-bundle3 -f /file3 "$TEST_NAME"

}

@test "bundle-remove remove multiple bundles each containing a file" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1 test-bundle2 test-bundle3"
	assert_status_is 0
	assert_file_not_exists "$TEST_NAME"/target-dir/foo/file1
	assert_file_not_exists "$TEST_NAME"/target-dir/bar/file2
	assert_file_not_exists "$TEST_NAME"/target-dir/file3
	assert_file_not_exists "$TEST_NAME"/target-dir/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$TEST_NAME"/target-dir/usr/share/clear/bundles/test-bundle2
	assert_file_not_exists "$TEST_NAME"/target-dir/usr/share/clear/bundles/test-bundle3
	assert_dir_not_exists "$TEST_NAME"/target-dir/foo
	assert_dir_not_exists "$TEST_NAME"/target-dir/bar
	expected_output=$(cat <<-EOM
		Removing bundle: test-bundle1
		Deleting bundle files...
		Total deleted files: 2
		Removing bundle: test-bundle2
		Deleting bundle files...
		Total deleted files: 2
		Removing bundle: test-bundle3
		Deleting bundle files...
		Total deleted files: 2
		Successfully removed 3 bundles
	EOM
	)
	assert_is_output "$expected_output"

}
