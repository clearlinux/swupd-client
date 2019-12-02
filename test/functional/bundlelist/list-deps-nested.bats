#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo "$TEST_NAME"
	create_bundle -n test-bundle2 -f /foo "$TEST_NAME"
	create_bundle -n test-bundle3 -f /foo "$TEST_NAME"
	# add bundle2 as dependencies of bundle1 and bundle 3 as dependency of bundle2
	add_dependency_to_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle1 test-bundle2
	add_dependency_to_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle2 test-bundle3

}

@test "LST007: List bundle's dependencies when bundle has nested dependencies" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --deps test-bundle1"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Bundles included by test-bundle1:
		 - test-bundle2
		 - test-bundle3

		Total: 2
	EOM
	)
	assert_is_output --identical "$expected_output"

}
