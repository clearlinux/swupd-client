#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME"
	# create some extra files in /usr
	sudo touch "$TARGETDIR"/usr/file1
	sudo mkdir -p "$TARGETDIR"/usr/foo/bar
	sudo touch "$TARGETDIR"/usr/foo/bar/file2
	# create extra file outside of /usr (should be ignored by picky)
	sudo touch "$TARGETDIR"/file3

}

@test "VER035: Verify a system that has extra files in /usr" {

	# --picky should find those files in /usr that are not in any manifest
	# but it should not delete them (unless --fix is also used)

	run sudo sh -c "$SWUPD verify --picky $SWUPD_OPTS"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Verifying version 10
		Generating list of extra files under .*/target-dir/usr
		/usr/share/defaults/swupd/versionurl
		/usr/share/defaults/swupd/contenturl
		/usr/foo/bar/file2
		/usr/foo/bar/
		/usr/foo/
		/usr/file1
		Inspected 17 files
		  6 files found which should be deleted
		Verify successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/file1
	assert_file_exists "$TARGETDIR"/usr/foo/bar/file2

}
