#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME"
	create_bundle -L -n test-bundle -f /usr/lib/kernel/test-file "$TEST_NAME"

}

@test "bundle-remove remove bundle containing a boot file" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle"
	assert_status_is 0
	assert_file_not_exists "$TARGETDIR"/usr/lib/kernel/testfile
	expected_output=$(cat <<-EOM
		Deleting bundle files...
		Total deleted files: 2
		Successfully removed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

}
