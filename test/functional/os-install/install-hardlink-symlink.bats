#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME" 10
	create_bundle -n os-core -h /core,/core2,/core3 -H /symlink,/symlink2,/symlink3 "$TEST_NAME"

}

@test "INS027: Check if hardlinks are preserved" {

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS --path $TARGETDIR"
	assert_status_is "$SWUPD_OK"
	run sudo sh -c "$SWUPD clean --all $SWUPD_OPTS --path $TARGETDIR"
	assert_status_is "$SWUPD_OK"

	run stat --printf=%h "$TARGETDIR"/core
	assert_status_is "0"
	assert_is_output "3"

	run stat --printf=%h "$TARGETDIR"/core2
	assert_status_is "0"
	assert_is_output "3"

	run stat --printf=%h "$TARGETDIR"/core3
	assert_status_is "0"
	assert_is_output "3"

}

@test "INS028: Check if symlink hardlinks are not preserved" {

	# There are known bugs on bsdtar to extract hardlink to symlinks,
	# so we avoid to have them on swupd.

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS --path $TARGETDIR"
	assert_status_is "$SWUPD_OK"
	run sudo sh -c "$SWUPD clean --all $SWUPD_OPTS --path $TARGETDIR"
	assert_status_is "$SWUPD_OK"

	run stat --printf=%h "$TARGETDIR"/symlink
	assert_status_is "0"
	assert_is_output "1"

	run stat --printf=%h "$TARGETDIR"/symlink2
	assert_status_is "0"
	assert_is_output "1"

	run stat --printf=%h "$TARGETDIR"/symlink3
	assert_status_is "0"
	assert_is_output "1"

}