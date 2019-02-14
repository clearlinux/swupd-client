#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

file_created=false

test_setup() {

	if [ -z "$TRAVIS" ]; then
		skip "This test is intended to run only in Travis (use TRAVIS=true to run it anyway)..."
	fi

	if [ ! -e /usr/bin/swupd-inspector ]; then
		print "swupd-inspector not found in the system, creating a mock one..."
		{
			printf '#!/bin/bash\n'
			printf 'echo "Inspect and download swupd content"\n'
			# shellcheck disable=SC2016
			printf 'echo "Fake inspector invoked successfully with these arguments: $1 $2 $3"\n'
		} | sudo tee /usr/bin/swupd-inspector > /dev/null
		sudo chmod +x /usr/bin/swupd-inspector
		file_created=true
	else
		print "swupd-inspector was found installed in the system"
	fi

}

test_teardown() {

	if [ -z "$TRAVIS" ]; then
		return
	fi

	if [ "$file_created" = true ]; then
		print "removing the mock swupd-inspector from the system..."
		sudo rm /usr/bin/swupd-inspector
	fi

}

@test "USA008: The external module swupd-inspector can be called from swupd" {

	# swupd should be able to run enabled external programs
	run sudo sh -c "$SWUPD inspector"

	assert_status_is 0
	assert_in_output "Inspect and download swupd content"

}

@test "USA009: Arguments are parsed correctly when calling external modules" {

	if [ "$file_created" = false ]; then
		skip "this test only makes sense using the mock swupd-inspector, skipping it..."
	fi

	run sudo sh -c "$SWUPD inspector arg1 arg2 arg3"

	assert_status_is 0
	assert_in_output "Fake inspector invoked successfully with these arguments: arg1 arg2 arg3"

}
