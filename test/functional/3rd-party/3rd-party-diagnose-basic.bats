#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

setup_file() {

	create_test_environment "$TEST_NAME" 10 1

	# add a 3rd-party repo with some "findings" for diagnose
	create_third_party_repo -a "$TEST_NAME" 10 1 test-repo1
	create_bundle -L -t -n test-bundle1 -f /foo/file_1,/bar/file_2 -u test-repo1 "$TEST_NAME"
	create_version -p "$TEST_NAME" 20 10 1 test-repo1
	update_bundle -p "$TEST_NAME" test-bundle1 --update /foo/file_1 test-repo1
	update_bundle -p "$TEST_NAME" test-bundle1 --delete /bar/file_2 test-repo1
	update_bundle    "$TEST_NAME" test-bundle1 --add    /baz/file_3 test-repo1
	set_current_version "$TEST_NAME" 20 test-repo1

	# add another 3rd-party repo that has nothing to get fixed
	create_third_party_repo -a "$TEST_NAME" 10 1 test-repo2
	create_bundle -L -t -n test-bundle2 -f /baz/file_3 -u test-repo2 "$TEST_NAME"

	# adding an untracked files into an untracked directory (/bat)
	sudo mkdir "$TARGET_DIR"/"$TP_BUNDLES_DIR"/test-repo1/bat
	sudo touch "$TARGET_DIR"/"$TP_BUNDLES_DIR"/test-repo1/bat/untracked_file1
	# adding an untracked file into tracked directory (/bar)
	sudo touch "$TARGET_DIR"/"$TP_BUNDLES_DIR"/test-repo1/bar/untracked_file2
	# adding an untracked file into /usr
	sudo touch "$TARGET_DIR"/"$TP_BUNDLES_DIR"/test-repo2/usr/untracked_file3

}

teardown_file() {

	destroy_test_environment --force "$TEST_NAME"

}

@test "TPR042: Diagnose shows modified files, new files and deleted files from 3rd-party bundles" {

	run sudo sh -c "$SWUPD 3rd-party diagnose $SWUPD_OPTS"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		__________________________________
		 3rd-Party Repository: test-repo1
		__________________________________
		Diagnosing version 20
		Downloading missing manifests...
		Checking for missing files
		 -> Missing file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/baz
		 -> Missing file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/baz/file_3
		Checking for corrupt files
		 -> Hash mismatch for file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/foo/file_1
		 -> Hash mismatch for file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/usr/lib/os-release
		Checking for extraneous files
		 -> File that should be deleted: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/bar/file_2
		Inspected 19 files
		  2 files were missing
		  2 files did not match
		  1 file found which should be deleted
		Use "swupd repair" to correct the problems in the system
		Diagnose successful
		__________________________________
		 3rd-Party Repository: test-repo2
		__________________________________
		Diagnosing version 10
		Downloading missing manifests...
		Checking for missing files
		Checking for corrupt files
		Checking for extraneous files
		Inspected 15 files
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR043: Diagnose 3rd-party bundles from a specific repo" {

	run sudo sh -c "$SWUPD 3rd-party diagnose $SWUPD_OPTS --repo test-repo2"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Downloading missing manifests...
		Checking for missing files
		Checking for corrupt files
		Checking for extraneous files
		Inspected 15 files
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR044: Try diagnose 3rd-party bundles to a specific version without specifying a repo" {

	run sudo sh -c "$SWUPD 3rd-party diagnose $SWUPD_OPTS --version 10"

	assert_status_is "$SWUPD_INVALID_OPTION"
	expected_output=$(cat <<-EOM
		Error: a repository needs to be specified to use the --version flag
	EOM
	)
	assert_in_output "$expected_output"

}

@test "TPR045: Diagnose 3rd-party bundles to a specific version" {

	run sudo sh -c "$SWUPD 3rd-party diagnose $SWUPD_OPTS --version 10 --repo test-repo1"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Diagnosing version 10
		Downloading missing manifests...
		Checking for missing files
		Checking for corrupt files
		 -> Hash mismatch for file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/usr/lib/os-release
		Checking for extraneous files
		Inspected 17 files
		  1 file did not match
		Use "swupd repair" to correct the problems in the system
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR046: Diagnose 3rd-party bundles using the picky option" {

	run sudo sh -c "$SWUPD 3rd-party diagnose $SWUPD_OPTS --picky"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		__________________________________
		 3rd-Party Repository: test-repo1
		__________________________________
		Diagnosing version 20
		Downloading missing manifests...
		Checking for missing files
		 -> Missing file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/baz
		 -> Missing file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/baz/file_3
		Checking for corrupt files
		 -> Hash mismatch for file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/foo/file_1
		 -> Hash mismatch for file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/usr/lib/os-release
		Checking for extraneous files
		 -> File that should be deleted: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/bar/file_2
		Checking for extra files under $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/usr
		Inspected 19 files
		  2 files were missing
		  2 files did not match
		  1 file found which should be deleted
		Use "swupd repair" to correct the problems in the system
		Diagnose successful
		__________________________________
		 3rd-Party Repository: test-repo2
		__________________________________
		Diagnosing version 10
		Downloading missing manifests...
		Checking for missing files
		Checking for corrupt files
		Checking for extraneous files
		Checking for extra files under $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo2/usr
		 -> Extra file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo2/usr/untracked_file3
		Inspected 16 files
		  1 file found which should be deleted
		Use "swupd repair --picky" to correct the problems in the system
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR047: Diagnose 3rd-party bundles using the picky option specifying the tree" {

	run sudo sh -c "$SWUPD 3rd-party diagnose $SWUPD_OPTS --picky --picky-tree /bat"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		__________________________________
		 3rd-Party Repository: test-repo1
		__________________________________
		Diagnosing version 20
		Downloading missing manifests...
		Checking for missing files
		 -> Missing file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/baz
		 -> Missing file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/baz/file_3
		Checking for corrupt files
		 -> Hash mismatch for file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/foo/file_1
		 -> Hash mismatch for file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/usr/lib/os-release
		Checking for extraneous files
		 -> File that should be deleted: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/bar/file_2
		Checking for extra files under $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/bat
		 -> Extra file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/bat/untracked_file1
		 -> Extra file: $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo1/bat/
		Inspected 21 files
		  2 files were missing
		  2 files did not match
		  3 files found which should be deleted
		Use "swupd repair --picky" to correct the problems in the system
		Diagnose successful
		__________________________________
		 3rd-Party Repository: test-repo2
		__________________________________
		Diagnosing version 10
		Downloading missing manifests...
		Checking for missing files
		Checking for corrupt files
		Checking for extraneous files
		Checking for extra files under $ABS_TARGET_DIR/$TP_BUNDLES_DIR/test-repo2/bat
		Inspected 15 files
		Diagnose successful
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=10
