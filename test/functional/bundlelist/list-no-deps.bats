#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo "$TEST_NAME"
	create_bundle -n test-bundle2 -f /foo "$TEST_NAME"

}

@test "LST008: List bundle's dependencies when bundle has no dependencies" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --deps test-bundle1"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Loading required manifests...

		No included bundles
	EOM
	)
	assert_is_output --identical "$expected_output"

}
