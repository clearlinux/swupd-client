#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME" 30123 20

}

@test "API001: info" {

	run sudo sh -c "$SWUPD info $SWUPD_OPTS --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		30123
	EOM
	)
	assert_is_output "$expected_output"

}
