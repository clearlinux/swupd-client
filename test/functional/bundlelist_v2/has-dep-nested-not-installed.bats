#!/usr/bin/env bats

load "../testlib"

setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo "$TEST_NAME"
	create_bundle -n test-bundle2 -f /foo "$TEST_NAME"
	create_bundle -n test-bundle3 -f /foo "$TEST_NAME"
	# add bundle1 as dependencies of bundle2 and bundle 2 as dependency of bundle3
	add_dependency_to_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle2 test-bundle1
	add_dependency_to_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle3 test-bundle2
	update_hashes_in_mom "$TEST_NAME"/web-dir/10/Manifest.MoM

}

teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "bundle-list list bundle has-deps with bundle not installed" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --has-dep test-bundle1"
	assert_status_is 13
	expected_output=$(cat <<-EOM
		Error: Bundle "test-bundle1" does not seem to be installed
		       try passing --all to check uninstalled bundles
		Error: Bundle list failed
	EOM
	)
	assert_in_output "$expected_output"

}
