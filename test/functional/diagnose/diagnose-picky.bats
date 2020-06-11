#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME"
	# create some extra files in /usr
	sudo touch "$TARGET_DIR"/usr/file1
	sudo mkdir -p "$TARGET_DIR"/usr/foo/bar
	sudo touch "$TARGET_DIR"/usr/foo/bar/file2
	# create extra file outside of /usr (should be ignored by picky)
	sudo touch "$TARGET_DIR"/file3

}

@test "DIA005: Diagnose a system that has extra files in /usr" {

	# --picky should find those files in /usr that are not in any manifest
	# but it should not delete them (unless --fix is also used)

	run sudo sh -c "$SWUPD diagnose --picky $SWUPD_OPTS"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Downloading missing manifests...
		Checking for missing files
		Checking for corrupt files
		Checking for extraneous files
		Checking for extra files under $ABS_TARGET_DIR/usr
		 -> Extra file: $ABS_TARGET_DIR/usr/share/defaults/swupd/versionurl
		 -> Extra file: $ABS_TARGET_DIR/usr/share/defaults/swupd/contenturl
		 -> Extra file: $ABS_TARGET_DIR/usr/foo/bar/file2
		 -> Extra file: $ABS_TARGET_DIR/usr/foo/bar/
		 -> Extra file: $ABS_TARGET_DIR/usr/foo/
		 -> Extra file: $ABS_TARGET_DIR/usr/file1
		Inspected 17 files
		  6 files found which should be deleted
		Use "swupd repair --picky" to correct the problems in the system
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGET_DIR"/usr/file1
	assert_file_exists "$TARGET_DIR"/usr/foo/bar/file2

}

@test "DIA015: Diagnose only for extra files in a system that has extra files in /usr" {

	# the --extra-files-only can be used to find those extra files and nothing else

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --extra-files-only"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Downloading missing manifests...
		Checking for extra files under $ABS_TARGET_DIR/usr
		 -> Extra file: $ABS_TARGET_DIR/usr/share/defaults/swupd/versionurl
		 -> Extra file: $ABS_TARGET_DIR/usr/share/defaults/swupd/contenturl
		 -> Extra file: $ABS_TARGET_DIR/usr/foo/bar/file2
		 -> Extra file: $ABS_TARGET_DIR/usr/foo/bar/
		 -> Extra file: $ABS_TARGET_DIR/usr/foo/
		 -> Extra file: $ABS_TARGET_DIR/usr/file1
		Inspected 6 files
		  6 files found which should be deleted
		Use "swupd repair --picky" to correct the problems in the system
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=4
