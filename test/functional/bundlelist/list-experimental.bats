#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -e -n test-bundle1 -f /file_1 "$TEST_NAME"
	create_bundle -e -n test-bundle2 -f /file_2 "$TEST_NAME"
	create_bundle -n test-bundle3 -f /file_3 "$TEST_NAME"

}

@test "LST014: List all available bundles including experimental" {

	# experimental bundles should be listed with something that distinguish them
	# from a normal bundle

	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --all"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		os-core
		test-bundle1 (experimental)
		test-bundle2 (experimental)
		test-bundle3
	EOM
	)
	assert_is_output --identical "$expected_output"
	# TODO(castulo): at the moment all that is expected is that swupd doesn't break
	# with the e modifier in the MoM, this test should be extended to identify what
	# bundles are experimental once it is implemented in swupd

}
