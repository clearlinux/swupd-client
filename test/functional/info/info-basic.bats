#!/usr/bin/env bats

# Author: Karthik Prabhu Vinod
# Email: karthik.prabhu.vinod@intel.com

load "../testlib"

metadata_setup() {

	create_test_environment "$TEST_NAME" 10 1

}

@test "INF001: Basic test, Check info verbose" {

	run sudo sh -c "$SWUPD info $SWUPD_OPTS --verbose"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Distribution:      Swupd Test Distro
		Installed version: 10 (format 1)
		Version URL:       file://$TEST_DIRNAME/web-dir
		Content URL:       file://$TEST_DIRNAME/web-dir
	EOM
	)

	assert_is_output "$expected_output"

}

@test "INF002: Basic test, Check info basic" {

	run sudo sh -c "$SWUPD info $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Distribution:      Swupd Test Distro
		Installed version: 10
		Version URL:       file://$TEST_DIRNAME/web-dir
		Content URL:       file://$TEST_DIRNAME/web-dir
	EOM
	)

	assert_is_output "$expected_output"

}
#WEIGHT=2
