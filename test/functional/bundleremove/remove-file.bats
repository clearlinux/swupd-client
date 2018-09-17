#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /test-file1 "$TEST_NAME"

}

@test "bundle-remove remove bundle containing a file" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1"
	assert_status_is 0
	assert_file_not_exists "$TEST_NAME"/target-dir/test-file1
	expected_output=$(cat <<-EOM
		Deleting bundle files...
		Total deleted files: 2
		Successfully removed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

}
