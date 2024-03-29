#!/usr/bin/env bats

load "../testlib"

setup_file() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo "$TEST_NAME"
	create_bundle -n test-bundle2 -f /foo "$TEST_NAME"
	create_bundle -n test-bundle3 -f /foo "$TEST_NAME"
	# add bundle2 and 3 as dependencies of bundle1
	add_dependency_to_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle1 test-bundle2
	add_dependency_to_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle1 test-bundle3

}

teardown_file() {

	destroy_test_environment --force "$TEST_NAME"

}

@test "LST006: List bundle's dependencies when bundle has flat dependencies" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --deps test-bundle1"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Loading required manifests...

		Bundles included by test-bundle1:
		 - os-core
		 - test-bundle2
		 - test-bundle3

		Total: 3
	EOM
	)
	assert_is_output --identical "$expected_output"

}
#WEIGHT=4
