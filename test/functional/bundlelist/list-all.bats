#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle       -n test-bundle1 -f /foo "$TEST_NAME"
	create_bundle       -n test-bundle2 -f /bar "$TEST_NAME"
	create_bundle -L    -n test-bundle3 -f /baz "$TEST_NAME"
	create_bundle -L -t -n test-bundle4 -f /bat "$TEST_NAME"

}

@test "LST002: List all available bundles" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --all"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		All available bundles:
		 - os-core
		 - test-bundle1
		 - test-bundle2
		 - test-bundle3
		 - test-bundle4

		Total: 5
	EOM
	)
	assert_is_output --identical "$expected_output"

}

@test "LST026: List all available bundles and their installation status" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --all --status"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		All available bundles:
		 - os-core (installed)
		 - test-bundle1
		 - test-bundle2
		 - test-bundle3 (installed)
		 - test-bundle4 (explicitly installed)

		Total: 5
	EOM
	)
	assert_is_output --identical "$expected_output"

}
