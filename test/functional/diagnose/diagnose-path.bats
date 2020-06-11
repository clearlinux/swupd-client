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
	update_bundle "$TEST_NAME" test-bundle1 --add /baz/file_3
	update_bundle "$TEST_NAME" test-bundle1 --add /bar/file_4
	update_bundle "$TEST_NAME" test-bundle1 --add /bar/bat/file_5
	set_current_version "$TEST_NAME" 20
	# adding an untracked files into an untracked directory (/bat)
	sudo mkdir "$TARGET_DIR"/bat
	sudo touch "$TARGET_DIR"/bat/untracked_file1
	# adding an untracked file into tracked directory (/bar)
	sudo touch "$TARGET_DIR"/bar/untracked_file2
	# adding an untracked file into /usr
	sudo touch "$TARGET_DIR"/usr/untracked_file3

}

@test "DIA022: Diagnose can be limited to a specific file" {

	# if the user uses the --file option, the diagnose is limited to the
	# file/directory specified by the user

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --file /baz/file_3"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Downloading missing manifests...
		Limiting diagnose to the following file:
		 - /baz/file_3
		Checking for missing files
		 -> Missing file: $ABS_TARGET_DIR/baz/file_3
		Checking for corrupt files
		Checking for extraneous files
		Inspected 1 file
		  1 file was missing
		Use "swupd repair" to correct the problems in the system
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "DIA023: Diagnose picky can be limited to a specific file" {

	# if the user uses the --file + --picky options, the diagnose is limited to the
	# file/directory specified by the user and looks for extra files in the
	# same path only

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --file /baz/file_3 --picky"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Downloading missing manifests...
		Limiting diagnose to the following file:
		 - /baz/file_3
		Checking for missing files
		 -> Missing file: $ABS_TARGET_DIR/baz/file_3
		Checking for corrupt files
		Checking for extraneous files
		Checking for extra files under $ABS_TARGET_DIR/baz/file_3
		Inspected 1 file
		  1 file was missing
		Use "swupd repair" to correct the problems in the system
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "DIA024: Diagnose can be limited to a specific path" {

	# if the user uses the --file option, the diagnose is limited to the
	# file/directory specified by the user

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --file /bar"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Downloading missing manifests...
		Limiting diagnose to the following directory (recursively):
		 - /bar
		Checking for missing files
		 -> Missing file: $ABS_TARGET_DIR/bar/bat
		 -> Missing file: $ABS_TARGET_DIR/bar/bat/file_5
		 -> Missing file: $ABS_TARGET_DIR/bar/file_4
		Checking for corrupt files
		Checking for extraneous files
		 -> File that should be deleted: $ABS_TARGET_DIR/bar/file_2
		Inspected 5 files
		  3 files were missing
		  1 file found which should be deleted
		Use "swupd repair" to correct the problems in the system
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "DIA025: Diagnose can be limited to a specific path to look for extra files only" {

	# if the user uses the --file + --extra-files-only option, swupd should only look for
	# extra files in the path specified by the user

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --file /bar --extra-files-only"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Downloading missing manifests...
		Limiting diagnose to the following directory (recursively):
		 - /bar
		Checking for extra files under $ABS_TARGET_DIR/bar
		 -> Extra file: $ABS_TARGET_DIR/bar/untracked_file2
		 -> Extra file: $ABS_TARGET_DIR/bar/file_2
		Inspected 2 files
		  2 files found which should be deleted
		Use "swupd repair --picky" to correct the problems in the system
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "DIA026: Diagnose can be limited to a specific path and can look for extra files in another path" {

	# if the user uses the --file option with --picky-tree,
	# the latter should take precedence

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --file /bar --picky --picky-tree /usr"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Downloading missing manifests...
		Limiting diagnose to the following directory (recursively):
		 - /bar
		Checking for missing files
		 -> Missing file: $ABS_TARGET_DIR/bar/bat
		 -> Missing file: $ABS_TARGET_DIR/bar/bat/file_5
		 -> Missing file: $ABS_TARGET_DIR/bar/file_4
		Checking for corrupt files
		Checking for extraneous files
		 -> File that should be deleted: $ABS_TARGET_DIR/bar/file_2
		Checking for extra files under $ABS_TARGET_DIR/usr
		 -> Extra file: $ABS_TARGET_DIR/usr/untracked_file3
		Inspected 6 files
		  3 files were missing
		  2 files found which should be deleted
		Use "swupd repair --picky" to correct the problems in the system
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "DIA027: Diagnose can be limited to a specific path for a specific bundle" {

	# if the --bundles and --file options are used together, swupd should only
	# look in the specified path and only for files that are part of the specified
	# bundle

	write_to_protected_file -a "$ABS_TARGET_DIR"/usr/share/clear/bundles/os-core "corrupting the file"
	write_to_protected_file -a "$ABS_TARGET_DIR"/usr/share/clear/bundles/test-bundle1 "corrupting the file"

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --bundle test-bundle1 --file /usr/share/clear/bundles"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Limiting diagnose to the following bundles:
		 - test-bundle1
		Downloading missing manifests...
		Limiting diagnose to the following directory (recursively):
		 - /usr/share/clear/bundles
		Checking for missing files
		Checking for corrupt files
		 -> Hash mismatch for file: $ABS_TARGET_DIR/usr/share/clear/bundles/os-core
		 -> Hash mismatch for file: $ABS_TARGET_DIR/usr/share/clear/bundles/test-bundle1
		Checking for extraneous files
		Inspected 3 files
		  2 files did not match
		Use "swupd repair" to correct the problems in the system
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "DIA028: Diagnose with --file doesn't consider partial matches" {

	# when using "diagnose --file <ITEM>", swupd will match the ITEM with
	# either the full name of a file or a directory (recursively), but won't
	# use partial matches

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --file /ba"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Downloading missing manifests...
		Limiting diagnose to the following file:
		 - /ba
		Inspected 0 files
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "DIA029: Diagnosing extra files only specifying an untracked file or directory" {

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --extra-files-only --file /usr/untracked_file3"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Downloading missing manifests...
		Limiting diagnose to the following file:
		 - /usr/untracked_file3
		Checking for extra files under $ABS_TARGET_DIR/usr/untracked_file3
		 -> Extra file: $ABS_TARGET_DIR/usr/untracked_file3
		Inspected 1 file
		  1 file found which should be deleted
		Use "swupd repair --picky" to correct the problems in the system
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}

#WEIGHT=36
