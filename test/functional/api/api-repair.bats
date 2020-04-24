#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME" 10 1
	add_os_core_update_bundle "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /foo/file_1,/bar/file_2 "$TEST_NAME"
	create_version "$TEST_NAME" 20 10 1
	update_bundle -p "$TEST_NAME" test-bundle1 --update /foo/file_1
	update_bundle -p "$TEST_NAME" test-bundle1 --delete /bar/file_2
	update_bundle "$TEST_NAME" test-bundle1 --add /baz/bat/file_3

}

test_teardown() {

	# return the files to mutable state
	if [ -e "$TARGETDIR"/usr/untracked_file ]; then
		sudo chattr -i "$TARGETDIR"/usr/untracked_file
	fi
	if [ -e "$TARGETDIR"/baz ]; then
		sudo chattr -i "$TARGETDIR"/baz
	fi

}

@test "API057: repair (nothing to repair)" {

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --picky --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty

}

@test "API058: repair" {

	# add things to be repaired
	set_current_version "$TEST_NAME" 20
	sudo touch "$TARGETDIR"/usr/untracked_file

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --picky --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		$PATH_PREFIX/baz
		$PATH_PREFIX/baz/bat
		$PATH_PREFIX/baz/bat/file_3
		$PATH_PREFIX/foo/file_1
		$PATH_PREFIX/usr/lib/os-release
		$PATH_PREFIX/bar/file_2
		$PATH_PREFIX/usr/untracked_file
	EOM
	)
	assert_is_output --identical "$expected_output"

}
#WEIGHT=17
