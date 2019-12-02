#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /file_1 "$TEST_NAME"
	create_bundle -L -n test-bundle2 -f /file_2 "$TEST_NAME"
	create_bundle -n test-bundle3 -f /file_3 "$TEST_NAME"

}

@test "LST021: List all installed bundles using a scriptable output (quiet)" {

	# when using the --quiet flag, the output should be in a format that
	# can be piped

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		os-core
		test-bundle1
		test-bundle2
	EOM
	)
	assert_is_output --identical "$expected_output"

}

@test "LST022: List all available bundles using scriptable output (quiet)" {

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --all  --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		os-core
		test-bundle1
		test-bundle2
		test-bundle3
	EOM
	)
	assert_is_output --identical "$expected_output"

}
