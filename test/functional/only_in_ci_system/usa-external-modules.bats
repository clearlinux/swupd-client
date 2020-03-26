#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

file_created=false

test_setup() {

	if [ -z "$RUNNING_IN_CI" ]; then
		skip "This test is intended to run only in Travis (use RUNNING_IN_CI=true to run it anyway)..."
	fi

	if [ ! -e /usr/bin/swupd-search ]; then
		print "swupd-search not found in the system, creating a mock one..."
		{
			printf '#!/bin/bash\n'
			printf 'echo "Bundle with the best search result:"\n'
			# shellcheck disable=SC2016
			printf 'echo "Fake search invoked successfully with these arguments: $1 $2 $3"\n'
		} | sudo tee /usr/bin/swupd-search > /dev/null
		sudo chmod +x /usr/bin/swupd-search
		file_created=true
	else
		print "swupd-search was found installed in the system"
	fi

}

test_teardown() {

	if [ -z "$RUNNING_IN_CI" ]; then
		return
	fi

	if [ "$file_created" = true ]; then
		print "removing the mock swupd-search from the system..."
		sudo rm /usr/bin/swupd-search
	fi

}

@test "USA008: The external module swupd-search can be called from swupd" {

	# swupd should be able to run enabled external programs
	run sudo sh -c "$SWUPD search os-core"

	assert_status_is 0
	assert_in_output "Bundle with the best search result:"

}

@test "USA009: Arguments are parsed correctly when calling external modules" {

	if [ "$file_created" = false ]; then
		skip "this test only makes sense using the mock swupd-search, skipping it..."
	fi

	run sudo sh -c "$SWUPD search arg1 arg2 arg3"

	assert_status_is 0
	assert_in_output "Fake search invoked successfully with these arguments: arg1 arg2 arg3"

}
#WEIGHT=1
