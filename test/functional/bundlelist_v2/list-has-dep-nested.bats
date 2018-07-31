#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /foo "$TEST_NAME"
	create_bundle -L -n test-bundle2 -f /foo "$TEST_NAME"
	create_bundle -L -n test-bundle3 -f /foo "$TEST_NAME"
	# add bundle1 as dependencies of bundle2 and bundle 2 as dependency of bundle3
	add_dependency_to_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle2 test-bundle1
	add_dependency_to_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle3 test-bundle2
	update_hashes_in_mom "$TEST_NAME"/web-dir/10/Manifest.MoM

}

@test "bundle-list list bundle has-deps with nested dependent bundles" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --has-dep test-bundle1"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Installed bundles that have test-bundle1 as a dependency:
		format:
		 # * is-required-by
		 #   |-- is-required-by
		 # * is-also-required-by
		 # ...
		  * test-bundle2
		    |-- test-bundle3
	EOM
	)
	assert_is_output "$expected_output"

}
