#!/usr/bin/env bats

# Author: Karthik Prabhu Vinod
# Email: karthik.prabhu.vinod@intel.com

load "../testlib"

test_setup() {

	if [ -z "TRAVIS"]; then
		skip "This test is intended to run only in Travis (use TRAVIS=true to run it anyway)..."
	fi

	create_test_environment "$TEST_NAME"

	run sudo sh -c "mkdir '$TEST_DIRNAME'/fakepath"
	# mock systemctl if it doesnt exist to always return true
	if [ ! -e /usr/bin/systemctl ]; then
		print "systemctl was not found installed in the system"
		{
			printf '#!/bin/bash\n'
			echo ''
		} | sudo tee /usr/bin/systemctl > /dev/null
		sudo chmod +x /usr/bin/systemctl
		file_created=true
	else
		print "systemctl was found installed in the system"
	fi

}

test_teardown() {

	if [ -z "$TRAVIS" ]; then
		return
	fi

	if [ "$file_created" = true ]; then
		print "removing mock systemctl from the system ..."
		sudo rm -r /usr/bin/systemctl
	fi

}

@test "AUP001: Basic test, Check auto-update enable" {

	run sudo sh -c "$SWUPD autoupdate --disable --path='$TEST_DIRNAME'/fakepath"

	expected_output=$(cat <<-EOM
		Warning: disabling automatic updates may take you out of compliance with your IT policy
		Running systemctl to disable updates
	EOM
	)

	assert_status_is "$SWUPD_OK"
	assert_is_output "$expected_output"

	run sudo sh -c "$SWUPD autoupdate --enable --path='$TEST_DIRNAME'/fakepath"

	assert_status_is "$SWUPD_OK"
	assert_is_output "Running systemctl to enable updates"

}
