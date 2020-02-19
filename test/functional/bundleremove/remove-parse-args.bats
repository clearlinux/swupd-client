#!/usr/bin/env bats

load "../testlib"

@test "REM013: Ensure bundle name is passed as an option when removing a bundle" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS"

	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: missing bundle(s) to be removed"

}
#WEIGHT=1
