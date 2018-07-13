#!/usr/bin/env bats

load "../testlib"

@test "bundle-remove remove bundle os-core" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS os-core"
	assert_status_is "$EBUNDLE_NOT_TRACKED"
	assert_file_exists "$TEST_NAME"/target-dir/usr/bin/core
	expected_output=$(cat <<-EOM
		Warning: Bundle "os-core" not allowed to be removed
		Failed to remove 1 of 1 bundles
	EOM
	)
	assert_in_output "$expected_output"

}