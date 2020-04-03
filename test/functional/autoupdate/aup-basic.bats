#!/usr/bin/env bats

# Author: Karthik Prabhu Vinod
# Email: karthik.prabhu.vinod@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"

}

@test "AUT001: auto-update disable-enable & status" {

	# perform autoupdate disable
	run sudo sh -c "$SWUPD autoupdate --disable $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: disabling automatic updates may take you out of compliance with your IT policy
		Running systemctl to disable updates
	EOM
	)
	assert_is_output "$expected_output"

	# check autoupdate status
	run sudo sh -c "$SWUPD autoupdate $SWUPD_OPTS"
	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Disabled
	EOM
	)
	assert_is_output "$expected_output"

	# check for files(should exist and point to /dev/null)
	assert_file_exists "$PATH_PREFIX/etc/systemd/system/swupd-update.service"
	assert_file_exists "$PATH_PREFIX/etc/systemd/system/swupd-update.timer"

	# perform autoupdate enable
	run sudo sh -c "$SWUPD autoupdate --enable $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Running systemctl to enable updates
		Warning: Running autoupdate with --path will not restart swupd-update.timer. This will have to be done manually
	EOM
	)
	assert_is_output "$expected_output"

	# check autoupdate enable
	# This still results in a Disabled result as it is happening inside
	# another file system. On root file system, this would have resulted in
	# an Enabled response
	run sudo sh -c "$SWUPD autoupdate $SWUPD_OPTS"
	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Disabled
	EOM
	)
	assert_is_output "$expected_output"


	# check for files(should not exist)
	assert_file_not_exists "$PATH_PREFIX/etc/systemd/system/swupd-update.service"
	assert_file_not_exists "$PATH_PREFIX/etc/systemd/system/swupd-update.timer"

}
#WEIGHT=2
