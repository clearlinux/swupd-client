#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /test-file "$TEST_NAME"

	# cache dir structure and permissions
	# <cachedir>
	# └── cache                                -> world readable
	#     └── url_based_dir                    -> world readable
	#         ├── delta                        -> root only
	#         ├── download                     -> root only
	#         ├── manifest                     -> world readable
	#         │   └── 10                       -> world readable
	#         ├── staged                       -> root only
	#         └── temp                         -> root only

}

@test "SIG009: Check if state dir permissions are correct after execution" {

	# remove any cache/data directories that may have been created by testlib
	sudo rm -rf "$STATE_DIR"

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

	assert_status_is "$SWUPD_OK"
	assert_file_exists "$TARGET_DIR"/test-file

	# cache
	run sudo sh -c "stat -c '%a' $ABS_STATE_DIR/cache"
	assert_status_is 0
	assert_in_output "755"

	# usr_baed_dir
	run sudo sh -c "stat -c '%a' $ABS_CACHE_DIR"
	assert_status_is 0
	assert_in_output "755"

	# delta
	run sudo sh -c "stat -c '%a' $ABS_DELTA_DIR"
	assert_status_is 0
	assert_in_output "700"

	# download
	run sudo sh -c "stat -c '%a' $ABS_DOWNLOAD_DIR"
	assert_status_is 0
	assert_in_output "700"

	# manifest
	run sudo sh -c "stat -c '%a' $ABS_MANIFEST_DIR"
	assert_status_is 0
	assert_in_output "755"
	run sudo sh -c "stat -c '%a' $ABS_MANIFEST_DIR/10"
	assert_status_is 0
	assert_in_output "755"

	# staged
	run sudo sh -c "stat -c '%a' $ABS_STAGED_DIR"
	assert_status_is 0
	assert_in_output "700"

	# temp
	run sudo sh -c "stat -c '%a' $ABS_TEMP_DIR"
	assert_status_is 0
	assert_in_output "700"

	# staged directory should only be accesible by root
	run sudo sh -c "stat -c '%a' $ABS_STAGED_DIR"
	assert_status_is 0
	assert_in_output "700"

}

@test "SIG028: Check if state dir permissions are correct after execution even if they are wrong before" {

	# remove any cache/data directories that may have been created by testlib
	sudo chmod -R 0755 "$STATE_DIR"

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

	assert_status_is "$SWUPD_OK"
	assert_file_exists "$TARGET_DIR"/test-file

	# cache
	run sudo sh -c "stat -c '%a' $ABS_STATE_DIR/cache"
	assert_status_is 0
	assert_in_output "755"

	# usr_baed_dir
	run sudo sh -c "stat -c '%a' $ABS_CACHE_DIR"
	assert_status_is 0
	assert_in_output "755"

	# delta
	run sudo sh -c "stat -c '%a' $ABS_DELTA_DIR"
	assert_status_is 0
	assert_in_output "700"

	# download
	run sudo sh -c "stat -c '%a' $ABS_DOWNLOAD_DIR"
	assert_status_is 0
	assert_in_output "700"

	# manifest
	run sudo sh -c "stat -c '%a' $ABS_MANIFEST_DIR"
	assert_status_is 0
	assert_in_output "755"
	run sudo sh -c "stat -c '%a' $ABS_MANIFEST_DIR/10"
	assert_status_is 0
	assert_in_output "755"

	# staged
	run sudo sh -c "stat -c '%a' $ABS_STAGED_DIR"
	assert_status_is 0
	assert_in_output "700"

	# temp
	run sudo sh -c "stat -c '%a' $ABS_TEMP_DIR"
	assert_status_is 0
	assert_in_output "700"

}

#WEIGHT=2
