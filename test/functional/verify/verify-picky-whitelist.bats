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
	sudo touch "$TARGETDIR"/usr/foo/bar/file3
	sudo mkdir -p "$TARGETDIR"/usr/bat/baz
	sudo touch "$TARGETDIR"/usr/bat/baz/file4
	sudo touch "$TARGETDIR"/usr/bat/baz/file5

}

@test "VER036: Verify directories in the whitelist are skipped during --picky" {

	# --picky should find those files in /usr that are not in any manifest
	# but it should skip those insde directories that are in the whitelist

	run sudo sh -c "$SWUPD verify --picky --picky-whitelist=/usr/foo $SWUPD_OPTS"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Verifying version 10
		Generating list of extra files under .*/target-dir/usr
		/usr/share/defaults/swupd/versionurl
		/usr/share/defaults/swupd/contenturl
		/usr/file1
		/usr/bat/baz/file5
		/usr/bat/baz/file4
		/usr/bat/baz/
		/usr/bat/
		Inspected 18 files
		  7 files found which should be deleted
		Verify successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/file1
	assert_file_exists "$TARGETDIR"/usr/foo/bar/file2

}
