#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

@test "REM032: Bundle remove conflicting flags" {

	# some flags are mutually exclusive

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS --orphans --force"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --orphans and --force options are mutually exclusive"

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS --orphans --recursive"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --orphans and --recursive options are mutually exclusive"

}
