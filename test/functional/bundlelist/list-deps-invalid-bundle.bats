#!/usr/bin/env bats

load "../testlib"

setup_file() {

	create_test_environment "$TEST_NAME"

}

teardown_file() {

	destroy_test_environment --force "$TEST_NAME"

}

@test "LST009: Try listing bundle's dependencies when bundle does not exist" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --deps not-a-bundle"

	assert_status_is "$SWUPD_INVALID_BUNDLE"
	expected_output=$(cat <<-EOM
		Loading required manifests...
		Warning: Bundle "not-a-bundle" is invalid, skipping it...
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=1
