#!/usr/bin/env bats

load "../testlib"

setup_file() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /foo "$TEST_NAME"
	create_bundle -L -n test-bundle2 -f /foo "$TEST_NAME"

}

teardown_file() {

	destroy_test_environment --force "$TEST_NAME"

}

@test "LST012: List bundles that have a given bundle as dependency (and there are none)" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --has-dep test-bundle1"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Loading required manifests...

		No bundles have test-bundle1 as a dependency
	EOM
	)
	assert_is_output --identical "$expected_output"

}
#WEIGHT=3
