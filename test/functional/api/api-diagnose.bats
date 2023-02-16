#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

setup_file() {

	create_test_environment -r "$TEST_NAME" 10 1
	add_os_core_update_bundle "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /foo/file_1,/bar/file_2 "$TEST_NAME"
	create_version "$TEST_NAME" 20 10 1
	update_bundle -p "$TEST_NAME" test-bundle1 --update /foo/file_1
	update_bundle -p "$TEST_NAME" test-bundle1 --delete /bar/file_2
	update_bundle "$TEST_NAME" test-bundle1 --add /baz/file_3

}

teardown_file() {

	destroy_test_environment --force "$TEST_NAME"

}

@test "API053: diagnose (no issues found)" {

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --picky --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty

}

@test "API054: diagnose (issues found)" {

	# set the current version of the target system as if it is already
	# at version 20 so diagnose find issues
	set_current_version "$TEST_NAME" 20
	# adding an untracked file into /usr
	sudo touch "$TARGET_DIR"/usr/untracked_file3

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --picky --quiet"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		$ABS_TARGET_DIR/baz
		$ABS_TARGET_DIR/baz/file_3
		$ABS_TARGET_DIR/foo/file_1
		$ABS_TARGET_DIR/usr/lib/os-release
		$ABS_TARGET_DIR/bar/file_2
		$ABS_TARGET_DIR/usr/untracked_file3
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=6
