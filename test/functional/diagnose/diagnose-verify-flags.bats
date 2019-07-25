#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

@test "Verify conflicting flags" {

	# Some flags are mutually exclusive

	# --quick & --picky
	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --quick --picky"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --quick and --picky options are mutually exclusive"

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --picky --quick"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --quick and --picky options are mutually exclusive"

	# --quick & --extra-files-only
	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --quick --extra-files-only"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --quick and --extra-files-only options are mutually exclusive"

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --extra-files-only --quick"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --quick and --extra-files-only options are mutually exclusive"

	# --picky & --extra-files-only
	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --picky --extra-files-only"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --extra-files-only and --picky options are mutually exclusive"

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --extra-files-only --picky"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --extra-files-only and --picky options are mutually exclusive"

	# --fix & --install
	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --fix --install --manifest latest"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --install and --fix options are mutually exclusive"

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --install --fix --manifest latest"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --install and --fix options are mutually exclusive"

	# --picky & --install
	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --picky --install --manifest latest"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --install and --picky options are mutually exclusive"

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --install --picky --manifest latest"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --install and --picky options are mutually exclusive"

	# --extra-files-only & --install
	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --extra-files-only --install --manifest latest"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --install and --extra-files-only options are mutually exclusive"

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --install --extra-files-only --manifest latest"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --install and --extra-files-only options are mutually exclusive"

	# --install without --manifest
	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --install"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --install option requires the --manifest option"

	# --manifest=latest without --installv
	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --manifest=latest"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: '--manifest latest' only supported with the --install option"

	# --bundles without --fix
	run sudo sh -c "$SWUPD verify $SWUPD_OPTS --bundles os-core"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --bundles option require the --fix option"

}
