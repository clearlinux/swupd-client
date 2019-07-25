#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

@test "REP008: Repair conflicting flags" {

	# Some flags are mutually exclusive

	# --quick & --picky
	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --quick --picky"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --quick and --picky options are mutually exclusive"

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --picky --quick"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --quick and --picky options are mutually exclusive"

	# --quick & --extra-files-only
	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --quick --extra-files-only"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --quick and --extra-files-only options are mutually exclusive"

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --extra-files-only --quick"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --quick and --extra-files-only options are mutually exclusive"

	# --picky & --extra-files-only
	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --picky --extra-files-only"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --extra-files-only and --picky options are mutually exclusive"

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --extra-files-only --picky"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --extra-files-only and --picky options are mutually exclusive"

}
