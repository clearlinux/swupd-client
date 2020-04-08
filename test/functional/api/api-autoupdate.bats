#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	sudo mkdir -p "$PATH_PREFIX"/etc/systemd/system

}

@test "API004: autoupdate" {

	run sudo sh -c "$SWUPD autoupdate $SWUPD_OPTS --quiet"

	assert_status_is "$SWUPD_NO"
	assert_output_is_empty

}

@test "API005: autoupdate --enable" {

	# create mask files manually
	sudo ln -s /dev/null "$PATH_PREFIX"/etc/systemd/system/swupd-update.service
	sudo ln -s /dev/null "$PATH_PREFIX"/etc/systemd/system/swupd-update.timer

	run sudo sh -c "$SWUPD autoupdate $SWUPD_OPTS --enable --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty
	# check mask files don't exist
	assert_file_not_exists "$PATH_PREFIX"/etc/systemd/system/swupd-update.service
	assert_file_not_exists "$PATH_PREFIX"/etc/systemd/system/swupd-update.timer

}

@test "API006: autoupdate --disable" {

	run sudo sh -c "$SWUPD autoupdate $SWUPD_OPTS --disable --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty
	# check mask files don't exist
	assert_symlink_exists "$PATH_PREFIX"/etc/systemd/system/swupd-update.service
	assert_symlink_exists "$PATH_PREFIX"/etc/systemd/system/swupd-update.timer

}

