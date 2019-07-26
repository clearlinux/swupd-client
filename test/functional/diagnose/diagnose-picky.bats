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

@test "DIA005: Diagnose a system that has extra files in /usr" {

	# --picky should find those files in /usr that are not in any manifest
	# but it should not delete them (unless --fix is also used)

	run sudo sh -c "$SWUPD diagnose --picky $SWUPD_OPTS"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Checking for missing files
		Checking for corrupt files
		Checking for extraneous files
		Checking for extra files under $PATH_PREFIX/usr
		 -> Extra file: $PATH_PREFIX/usr/share/defaults/swupd/versionurl
		 -> Extra file: $PATH_PREFIX/usr/share/defaults/swupd/contenturl
		 -> Extra file: $PATH_PREFIX/usr/foo/bar/file2
		 -> Extra file: $PATH_PREFIX/usr/foo/bar/
		 -> Extra file: $PATH_PREFIX/usr/foo/
		 -> Extra file: $PATH_PREFIX/usr/file1
		Inspected 17 files
		  6 files found which should be deleted
		Use "swupd repair --picky" to correct the problems in the system
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGETDIR"/usr/file1
	assert_file_exists "$TARGETDIR"/usr/foo/bar/file2

}

@test "DIA015: Diagnose only for extra files in a system that has extra files in /usr" {

	# the --extra-files-only can be used to find those extra files and nothing else

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --extra-files-only"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Checking for extra files under $PATH_PREFIX/usr
		 -> Extra file: $PATH_PREFIX/usr/share/defaults/swupd/versionurl
		 -> Extra file: $PATH_PREFIX/usr/share/defaults/swupd/contenturl
		 -> Extra file: $PATH_PREFIX/usr/foo/bar/file2
		 -> Extra file: $PATH_PREFIX/usr/foo/bar/
		 -> Extra file: $PATH_PREFIX/usr/foo/
		 -> Extra file: $PATH_PREFIX/usr/file1
		Inspected 6 files
		  6 files found which should be deleted
		Use "swupd repair --picky" to correct the problems in the system
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}
