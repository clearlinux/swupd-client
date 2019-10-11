#!/usr/bin/env bats

# Author: Karthik Prabhu Vinod
# Email: karthik.prabhu.vinod@intel.com

load "../testlib"

test_setup() {

	if [ -z "$TRAVIS" ]; then
		skip "This test is intended to run only in Travis (use TRAVIS=true to run it anyway)..."
	fi

	create_test_environment "$TEST_NAME"

	if [ ! -e /usr/bin/systemctl ]; then
		print "systemctl was not found installed in the system"
		return
	else
		print "systemctl was found installed in the system"
	fi

}

test_teardown() {

	if [ -z "$TRAVIS" ]; then
		return
	fi

}

@test "AUT001: Basic test, Check auto-update enable" {

	# perform autoupdate disable
	run sudo sh -c "$SWUPD autoupdate --disable $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Warning: disabling automatic updates may take you out of compliance with your IT policy
		Running systemctl to disable updates
	EOM
	)
	assert_is_output "$expected_output"

	# check autoupdate status after disable
	#run sudo sh -c "$SWUPD autoupdate $SWUPD_OPTS"
	#assert_status_is "$SWUPD_OK"
	#assert_is_output "Disabled"

	# check for files(should exist)
	# We cant use assert_file_exists here as it is symlinked to /dev/null
	run sudo sh -c "ls -l '$PATH_PREFIX'/etc/systemd/system/swupd-update.service"
	assert_status_is "$SWUPD_OK"
	run sudo sh -c "ls -l '$PATH_PREFIX'/etc/systemd/system/swupd-update.timer"
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
	run sudo sh -c "ls -l '$PATH_PREFIX'/etc/systemd/system/swupd-update.service"
	assert_status_is_not "$SWUPD_OK"
	run sudo sh -c "ls -l '$PATH_PREFIX'/etc/systemd/system/swupd-update.timer"
	assert_status_is_not "$SWUPD_OK"

	# check autoupdate status after enable
	#run sudo sh -c "$SWUPD autoupdate $SWUPD_OPTS"
	#assert_status_is "$SWUPD_OK"
	#expected_output=$(cat <<-EOM
	#	Disabled
	#EOM
	#)
	#assert_is_output "$expected_output"

}
