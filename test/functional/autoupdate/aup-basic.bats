#!/usr/bin/env bats

# Author: Karthik Prabhu Vinod
# Email: karthik.prabhu.vinod@intel.com

load "../testlib"

test_setup() {

	skip "This test is broken on github actions"

	create_test_environment "$TEST_NAME"

}

test_teardown() {

	if [ -z "$TRAVIS" ]; then
		return
	fi

	destroy_test_environment "$TEST_NAME"

}

@test "AUT001: Basic test, Check auto-update enable" {

	# perform autoupdate disable
	run sudo sh -c "$SWUPD autoupdate --disable --path=$PATH_PREFIX"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: disabling automatic updates may take you out of compliance with your IT policy
		Running systemctl to disable updates
	EOM
	)
	assert_is_output "$expected_output"

	# check for files(should exist and point to /dev/null)
	run sudo sh -c "test -c $PATH_PREFIX/etc/systemd/system/swupd-update.service"
	assert_status_is "$SWUPD_OK"
	run sudo sh -c "test -c $PATH_PREFIX/etc/systemd/system/swupd-update.timer"
	assert_status_is "$SWUPD_OK"

	# perform autoupdate enable
	run sudo sh -c "$SWUPD autoupdate --enable $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Running systemctl to enable updates
		Warning: Running autoupdate with --path will not restart swupd-update.timer. This will have to be done manually
	EOM
	)
	assert_is_output "$expected_output"

	# check for files(should not exist)
	run sudo sh -c "test -c $PATH_PREFIX/etc/systemd/system/swupd-update.service"
	assert_status_is_not "$SWUPD_OK"
	run sudo sh -c "test -c $PATH_PREFIX/etc/systemd/system/swupd-update.timer"
	assert_status_is_not "$SWUPD_OK"

}
