#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

setup_file() {

	create_test_environment "$TEST_NAME"

}

teardown_file() {

	destroy_test_environment --force "$TEST_NAME"

}

@test "MIR007: Trying to set a http mirror" {

	run sudo sh -c "$SWUPD mirror -s http://invalid_server_for_swupd $SWUPD_OPTS"

	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: This is an insecure connection
		Use the --allow-insecure-http flag to proceed
	EOM
	)
	assert_in_output "$expected_output"

}

@test "MIR008: Forcing a http mirror ang checking if the mirror works" {

	run sudo sh -c "$SWUPD mirror -s http://invalid_server_for_swupd --allow-insecure-http $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Warning: This is an insecure connection
		The --allow-insecure-http flag was used, be aware that this poses a threat to the system
		Warning: The mirror was set up using HTTP. In order for autoupdate to continue working you will need to set allow_insecure_http=true in the swupd configuration file. Alternatively you can set the mirror using HTTPS (recommended)
		Mirror url set
		Distribution:      Swupd Test Distro
		Installed version: 10
		Version URL:       http://invalid_server_for_swupd
		Content URL:       http://invalid_server_for_swupd
	EOM
	)
	assert_is_output "$expected_output"
	assert_equal "http://invalid_server_for_swupd" "$(<"$TARGET_DIR"/etc/swupd/mirror_contenturl)"
	assert_equal "http://invalid_server_for_swupd" "$(<"$TARGET_DIR"/etc/swupd/mirror_versionurl)"
}

@test "MIR009: swupd operations when http mirror is used without allow-insecure-http" {

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS -u http://invalid_server_for_swupd"

	assert_status_is "$SWUPD_INIT_GLOBALS_FAILED"
	expected_output=$(cat <<-EOM
		Error: This is an insecure connection
		Use the --allow-insecure-http flag to proceed
	EOM
	)
	assert_in_output "$expected_output"
}

@test "MIR010: swupd operations when http mirror is used without allow-insecure-http" {

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --allow-insecure-http -u http://invalid_server_for_swupd"

	# Error is because server doesn't respond to this manifest, but connection was created
	assert_status_is_not "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: This is an insecure connection
		The --allow-insecure-http flag was used, be aware that this poses a threat to the system
	EOM
	)
	assert_in_output "$expected_output"
}
#WEIGHT=2
