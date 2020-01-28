#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

@test "DIA016: Diagnose conflicting flags" {

	# Some flags are mutually exclusive

	# --picky & --extra-files-only
	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --picky --extra-files-only"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --extra-files-only and --picky options are mutually exclusive"

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --extra-files-only --picky"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --extra-files-only and --picky options are mutually exclusive"

	# --version with 'latest'
	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --version=latest"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: 'latest' not supported for --version"

	# --fix not supported
	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --fix"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "diagnose: unrecognized option '--fix'"

	# --install not supported
	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --install"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "diagnose: unrecognized option '--install'"

	# --bundles with --picky
	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --bundles vim --picky"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --bundles and --picky options are mutually exclusive"

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --picky --bundles vim"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --bundles and --picky options are mutually exclusive"

	# --bundles with --extra-files-only
	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --bundles vim --extra-files-only"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --bundles and --extra-files-only options are mutually exclusive"

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --extra-files-only --bundles vim"
	assert_status_is "$SWUPD_INVALID_OPTION"
	assert_in_output "Error: --bundles and --extra-files-only options are mutually exclusive"

}
