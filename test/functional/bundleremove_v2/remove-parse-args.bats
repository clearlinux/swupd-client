#!/usr/bin/env bats

load "../testlib"

@test "bundle-remove ensure bundle name is passed as an option" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS"
	assert_status_is "$EINVALID_OPTION"
	assert_in_output "error: missing bundle(s) to be removed"

}
