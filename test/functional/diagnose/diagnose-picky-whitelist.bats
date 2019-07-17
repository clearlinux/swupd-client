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

@test "DIA003: Diagnose directories in the whitelist are skipped during --picky" {

	# --picky should find those files in /usr that are not in any manifest
	# but it should skip those insde directories that are in the whitelist

	run sudo sh -c "$SWUPD diagnose --picky --picky-whitelist=/usr/foo $SWUPD_OPTS"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Checking for missing files
		Checking for corrupt files
		Checking for extraneous files
		Checking for extra files under .*/target-dir/usr
		 -> Extra file: /usr/share/defaults/swupd/versionurl
		 -> Extra file: /usr/share/defaults/swupd/contenturl
		 -> Extra file: /usr/file1
		 -> Extra file: /usr/bat/baz/file5
		 -> Extra file: /usr/bat/baz/file4
		 -> Extra file: /usr/bat/baz/
		 -> Extra file: /usr/bat/
		Inspected 18 files
		  7 files found which should be deleted
		Use "swupd repair --picky" to correct the problems in the system
		Diagnose successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/file1
	assert_file_exists "$TARGETDIR"/usr/foo/bar/file2

}
