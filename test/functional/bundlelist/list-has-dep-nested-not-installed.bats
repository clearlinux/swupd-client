#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo "$TEST_NAME"
	create_bundle -n test-bundle2 -f /foo "$TEST_NAME"
	create_bundle -n test-bundle3 -f /foo "$TEST_NAME"
	# add bundle1 as dependencies of bundle2 and bundle 2 as dependency of bundle3
	add_dependency_to_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle2 test-bundle1
	add_dependency_to_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle3 test-bundle2

}

@test "LST013: Try listing bundles that have a given bundle as dependency (and the bundle is not installed)" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --has-dep test-bundle1"

	assert_status_is "$SWUPD_BUNDLE_NOT_TRACKED"
	expected_output=$(cat <<-EOM
		Bundle "test-bundle1" does not seem to be installed
		       try passing --all to check uninstalled bundles
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=3
