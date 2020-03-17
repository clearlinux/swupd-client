#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

metadata_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /test-file "$TEST_NAME"

}

@test "SIG009: Check if state dir permissions are correct after execution" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

	assert_status_is "$SWUPD_OK"
	assert_file_exists "$TARGETDIR"/test-file

	run sudo sh -c "stat -c '%a' $STATEDIR"
	assert_status_is 0
	assert_in_output "700"

}
#WEIGHT=2
