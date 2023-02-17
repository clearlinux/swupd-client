#!/usr/bin/env bats

load "../testlib"

setup_file() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo "$TEST_NAME"
	create_bundle -n test-bundle2 -f /foo "$TEST_NAME"
	create_bundle -n test-bundle3 -f /foo "$TEST_NAME"
	# add bundle1 as dependencies of bundle2 and bundle 2 as dependency of bundle3
	add_dependency_to_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle2 test-bundle1
	add_dependency_to_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle3 test-bundle2

}

teardown_file() {

	destroy_test_environment --force "$TEST_NAME"

}

@test "LST010: List all bundles that have a given bundle as dependency" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --has-dep test-bundle1 --all"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Loading required manifests...

		All bundles that have test-bundle1 as a dependency:
		 - test-bundle2
		 - test-bundle3

		Total: 2
	EOM
	)
	assert_is_output --identical "$expected_output"

}
#WEIGHT=4
