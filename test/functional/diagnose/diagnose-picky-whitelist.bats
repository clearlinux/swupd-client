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
	sudo touch "$TARGET_DIR"/usr/foo/bar/file3
	sudo mkdir -p "$TARGET_DIR"/usr/bat/baz
	sudo touch "$TARGET_DIR"/usr/bat/baz/file4
	sudo touch "$TARGET_DIR"/usr/bat/baz/file5

}

@test "DIA003: Diagnose directories in the whitelist are skipped during --picky" {

	# --picky should find those files in /usr that are not in any manifest
	# but it should skip those insde directories that are in the whitelist

	run sudo sh -c "$SWUPD diagnose --picky --picky-whitelist=/usr/foo $SWUPD_OPTS"

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
		 -> Extra file: $ABS_TARGET_DIR/usr/file1
		 -> Extra file: $ABS_TARGET_DIR/usr/bat/baz/file5
		 -> Extra file: $ABS_TARGET_DIR/usr/bat/baz/file4
		 -> Extra file: $ABS_TARGET_DIR/usr/bat/baz/
		 -> Extra file: $ABS_TARGET_DIR/usr/bat/
		Inspected 18 files
		  7 files found which should be deleted
		Use "swupd repair --picky" to correct the problems in the system
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_exists "$TARGET_DIR"/usr/file1
	assert_file_exists "$TARGET_DIR"/usr/foo/bar/file2

}
#WEIGHT=2
