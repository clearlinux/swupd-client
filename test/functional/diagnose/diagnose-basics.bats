#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME" 10 1
	create_bundle -L -n test-bundle1 -f /foo/file_1,/bar/file_2 "$TEST_NAME"
	create_version "$TEST_NAME" 20 10 1
	update_bundle -p "$TEST_NAME" test-bundle1 --update /foo/file_1
	update_bundle -p "$TEST_NAME" test-bundle1 --delete /bar/file_2
	update_bundle "$TEST_NAME" test-bundle1 --add /baz/file_3
	set_current_version "$TEST_NAME" 20
	# adding an untracked files into an untracked directory (/bat)
	sudo mkdir "$TARGETDIR"/bat
	sudo touch "$TARGETDIR"/bat/untracked_file1
	# adding an untracked file into tracked directory (/bar)
	sudo touch "$TARGETDIR"/bar/untracked_file2
	# adding an untracked file into /usr
	sudo touch "$TARGETDIR"/usr/untracked_file3

}

@test "DIA012: Diagnose shows modified files, new files and deleted files" {

	# verify should show tracked files with a hash mismatch,
	# it should also show tracked files that were marked to be removed,
	# and it should show files that were added to bundles.
	# verify should not show any of these changes in untracked files

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Download missing manifests
		Checking for missing files
		.* Missing file: .*/target-dir/baz
		.* Missing file: .*/target-dir/baz/file_3
		Checking for corrupt files
		.* Hash mismatch for file: .*/target-dir/foo/file_1
		.* Hash mismatch for file: .*/target-dir/usr/lib/os-release
		Checking for extraneous files
		.* File that should be deleted: .*/target-dir/bar/file_2
		Inspected 18 files
		  2 files were missing
		  2 files did not match
		  1 file found which should be deleted
		Use "swupd repair" to correct the problems in the system
		Diagnose successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	# tracked files
	assert_file_exists "$TARGETDIR"/foo/file_1
	assert_file_exists "$TARGETDIR"/bar/file_2
	assert_file_not_exists "$TARGETDIR"/baz/file_3
	# untracked files
	assert_file_exists "$TARGETDIR"/bat/untracked_file1
	assert_file_exists "$TARGETDIR"/bar/untracked_file2
	assert_file_exists "$TARGETDIR"/usr/untracked_file3

}
