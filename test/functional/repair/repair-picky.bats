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

@test "REP006: Repair a system that has extra files in /usr" {

	# --picky should find those files in /usr that are not in any manifest
	# and delete them along with the other normal repairs

	run sudo sh -c "$SWUPD repair --picky $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Downloading missing manifests...
		Checking for corrupt files
		Adding any missing files
		Repairing corrupt files
		Removing extraneous files
		Removing extra files under $PATH_PREFIX/usr
		 -> Extra file: $PATH_PREFIX/usr/share/defaults/swupd/versionurl -> deleted
		 -> Extra file: $PATH_PREFIX/usr/share/defaults/swupd/contenturl -> deleted
		 -> Extra file: $PATH_PREFIX/usr/foo/bar/file2 -> deleted
		 -> Extra file: $PATH_PREFIX/usr/foo/bar/ -> deleted
		 -> Extra file: $PATH_PREFIX/usr/foo/ -> deleted
		 -> Extra file: $PATH_PREFIX/usr/file1 -> deleted
		Inspected 17 files
		  6 files found which should be deleted
		    6 of 6 files were deleted
		    0 of 6 files were not deleted
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/usr/file1
	assert_file_not_exists "$TARGETDIR"/usr/foo/bar/file2

}

@test "REP007: Repair removes only extra files in a system that has extra files in /usr" {

	# the --extra-files-only can be used to remove those extra files and nothing else

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --extra-files-only"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Downloading missing manifests...
		Removing extra files under $PATH_PREFIX/usr
		 -> Extra file: $PATH_PREFIX/usr/share/defaults/swupd/versionurl -> deleted
		 -> Extra file: $PATH_PREFIX/usr/share/defaults/swupd/contenturl -> deleted
		 -> Extra file: $PATH_PREFIX/usr/foo/bar/file2 -> deleted
		 -> Extra file: $PATH_PREFIX/usr/foo/bar/ -> deleted
		 -> Extra file: $PATH_PREFIX/usr/foo/ -> deleted
		 -> Extra file: $PATH_PREFIX/usr/file1 -> deleted
		Inspected 6 files
		  6 files found which should be deleted
		    6 of 6 files were deleted
		    0 of 6 files were not deleted
		Calling post-update helper scripts
		Repair successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/usr/file1
	assert_file_not_exists "$TARGETDIR"/usr/foo/bar/file2

}
