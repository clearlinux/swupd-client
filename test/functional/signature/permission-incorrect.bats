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
	sudo rm -rf "$STATEDIR"

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

	assert_status_is "$SWUPD_OK"
	assert_file_exists "$TARGETDIR"/test-file

	# cache
	run sudo sh -c "stat -c '%a' $STATEDIR_ABS/cache"
	assert_status_is 0
	assert_in_output "755"

	# usr_baed_dir
	run sudo sh -c "stat -c '%a' $STATEDIR_CACHE"
	assert_status_is 0
	assert_in_output "755"

	# delta
	run sudo sh -c "stat -c '%a' $STATEDIR_DELTA"
	assert_status_is 0
	assert_in_output "700"

	# download
	run sudo sh -c "stat -c '%a' $STATEDIR_DOWNLOAD"
	assert_status_is 0
	assert_in_output "700"

	# manifest
	run sudo sh -c "stat -c '%a' $STATEDIR_MANIFEST"
	assert_status_is 0
	assert_in_output "755"
	run sudo sh -c "stat -c '%a' $STATEDIR_MANIFEST/10"
	assert_status_is 0
	assert_in_output "755"

	# staged
	run sudo sh -c "stat -c '%a' $STATEDIR_STAGED"
	assert_status_is 0
	assert_in_output "700"

	# temp
	run sudo sh -c "stat -c '%a' $STATEDIR_TEMP"
	assert_status_is 0
	assert_in_output "700"

}

@test "SIG028: Check if state dir permissions are correct after execution even if they are wrong before" {

	# remove any cache/data directories that may have been created by testlib
	sudo chmod -R 0755 "$STATEDIR"

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

	assert_status_is "$SWUPD_OK"
	assert_file_exists "$TARGETDIR"/test-file

	# cache
	run sudo sh -c "stat -c '%a' $STATEDIR_ABS/cache"
	assert_status_is 0
	assert_in_output "755"

	# usr_baed_dir
	run sudo sh -c "stat -c '%a' $STATEDIR_CACHE"
	assert_status_is 0
	assert_in_output "755"

	# delta
	run sudo sh -c "stat -c '%a' $STATEDIR_DELTA"
	assert_status_is 0
	assert_in_output "700"

	# download
	run sudo sh -c "stat -c '%a' $STATEDIR_DOWNLOAD"
	assert_status_is 0
	assert_in_output "700"

	# manifest
	run sudo sh -c "stat -c '%a' $STATEDIR_MANIFEST"
	assert_status_is 0
	assert_in_output "755"
	run sudo sh -c "stat -c '%a' $STATEDIR_MANIFEST/10"
	assert_status_is 0
	assert_in_output "755"

	# staged
	run sudo sh -c "stat -c '%a' $STATEDIR_STAGED"
	assert_status_is 0
	assert_in_output "700"

	# temp
	run sudo sh -c "stat -c '%a' $STATEDIR_TEMP"
	assert_status_is 0
	assert_in_output "700"

}

#WEIGHT=2
