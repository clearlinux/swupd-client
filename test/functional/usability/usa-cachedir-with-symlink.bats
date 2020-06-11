#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"

	# create a symlink to the the statedir
	sudo ln -s "$ABS_STATE_DIR" "$ABS_TEST_DIR"/state_path
	sudo mkdir "$ABS_STATE_DIR"/new_state

}

@test "USA020: Tar files can be extracted into paths that contain symlinks" {

	# swupd should be able to extract files in the cache dir even when the path
	# provided for the cache directory contains symlinks

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS_NO_STATE --statedir $ABS_TEST_DIR/state_path/new_state"
	assert_status_is "$SWUPD_OK"

}
