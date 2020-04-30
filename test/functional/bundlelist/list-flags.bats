#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

@test "LST029: List conflicting flags" {

	# Some flags are mutually exclusive

	# --orphans & --all
	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --orphans --all"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --orphans and --all options are mutually exclusive"

	# --orphans & --has-dep
	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --orphans --has-dep os-core"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --orphans and --has-dep options are mutually exclusive"

	# --orphans & --deps
	run sudo sh -c "$SWUPD bundle-list $SWUPD_OPTS --orphans --deps 0s-core"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --orphans and --deps options are mutually exclusive"

}
