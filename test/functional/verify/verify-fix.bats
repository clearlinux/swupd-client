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

@test "VER031: Verify shows modified files, new files and deleted files" {

	# verify should show tracked files with a hash mismatch,
	# it should also show tracked files that were marked to be removed,
	# and it should show files that were added to bundles.
	# verify should not show any of these changes in untracked files

	run sudo sh -c "$SWUPD verify $SWUPD_OPTS"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		Verifying version 20
		Verifying files
		Missing file: .*/target-dir/baz
		Missing file: .*/target-dir/baz/file_3
		Hash mismatch for file: .*/target-dir/foo/file_1
		Hash mismatch for file: .*/target-dir/usr/lib/os-release
		File that should be deleted: .*/target-dir/bar/file_2
		Inspected 18 files
		  2 files were missing
		  2 files did not match
		  1 file found which should be deleted
		Verify successful
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

@test "VER032: Verify fixes modified files, new files and deleted files" {

	# verify --fix should fix tracked files with a hash mismatch,
	# it should also delete tracked files that were marked to be removed,
	# and it should add files that were added to bundles.
	# verify --fix should not delete any untracked files

	run sudo sh -c "$SWUPD verify --fix $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 20
		Verifying files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		Missing file: .*/target-dir/baz
		.fixed
		Missing file: .*/target-dir/baz/file_3
		.fixed
		Fixing modified files
		Hash mismatch for file: .*/target-dir/foo/file_1
		.fixed
		Hash mismatch for file: .*/target-dir/usr/lib/os-release
		.fixed
		File that should be deleted: .*/target-dir/bar/file_2
		.deleted
		Inspected 18 files
		  2 files were missing
		    2 of 2 missing files were replaced
		    0 of 2 missing files were not replaced
		  2 files did not match
		    2 of 2 files were fixed
		    0 of 2 files were not fixed
		  1 file found which should be deleted
		    1 of 1 files were deleted
		    0 of 1 files were not deleted
		Calling post-update helper scripts.
		Fix successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	# tracked files
	assert_file_exists "$TARGETDIR"/foo/file_1
	assert_file_not_exists "$TARGETDIR"/bar/file_2
	assert_file_exists "$TARGETDIR"/baz/file_3
	# untracked files
	assert_file_exists "$TARGETDIR"/bat/untracked_file1
	assert_file_exists "$TARGETDIR"/bar/untracked_file2
	assert_file_exists "$TARGETDIR"/usr/untracked_file3

}

@test "VER033: Verify fixes modified files, new files, deleted files, and removes untracked files" {

	# verify --fix --picky should fix tracked files with a hash mismatch,
	# it should also delete tracked files that were marked to be removed,
	# and it should add files that were added to bundles.
	# verify --fix --picky should delete any untracked files within /usr

	run sudo sh -c "$SWUPD verify --fix --picky $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 20
		Verifying files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		Missing file: .*/target-dir/baz
		.fixed
		Missing file: .*/target-dir/baz/file_3
		.fixed
		Fixing modified files
		Hash mismatch for file: .*/target-dir/foo/file_1
		.fixed
		Hash mismatch for file: .*/target-dir/usr/lib/os-release
		.fixed
		File that should be deleted: .*/target-dir/bar/file_2
		.deleted
		--picky removing extra files under .*/target-dir/usr
		REMOVING /usr/untracked_file3
		REMOVING /usr/share/defaults/swupd/versionurl
		REMOVING /usr/share/defaults/swupd/contenturl
		Inspected 21 files
		  2 files were missing
		    2 of 2 missing files were replaced
		    0 of 2 missing files were not replaced
		  2 files did not match
		    2 of 2 files were fixed
		    0 of 2 files were not fixed
		  4 files found which should be deleted
		    4 of 4 files were deleted
		    0 of 4 files were not deleted
		Calling post-update helper scripts.
		Fix successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	# tracked files
	assert_file_exists "$TARGETDIR"/foo/file_1
	assert_file_not_exists "$TARGETDIR"/bar/file_2
	assert_file_exists "$TARGETDIR"/baz/file_3
	# untracked files
	assert_file_exists "$TARGETDIR"/bat/untracked_file1
	assert_file_exists "$TARGETDIR"/bar/untracked_file2
	assert_file_not_exists "$TARGETDIR"/usr/untracked_file3
	assert_file_not_exists "$TARGETDIR"/usr/share/defaults/swupd/versionurl
	assert_file_not_exists "$TARGETDIR"/usr/share/defaults/swupd/contenturl

}

@test "VER034: Verify fixes modified files, new files, deleted files, and removes untracked files from a specified location" {

	# verify --fix --picky should fix tracked files with a hash mismatch,
	# it should also delete tracked files that were marked to be removed,
	# and it should add files that were added to bundles.
	# verify --fix --picky-tree=/bat should delete any untracked files within /bat

	run sudo sh -c "$SWUPD verify --fix --picky --picky-tree=/bat $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Verifying version 20
		Verifying files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		Missing file: .*/target-dir/baz
		.fixed
		Missing file: .*/target-dir/baz/file_3
		.fixed
		Fixing modified files
		Hash mismatch for file: .*/target-dir/foo/file_1
		.fixed
		Hash mismatch for file: .*/target-dir/usr/lib/os-release
		.fixed
		File that should be deleted: .*/target-dir/bar/file_2
		.deleted
		--picky removing extra files under .*/target-dir/bat
		REMOVING /bat/untracked_file1
		REMOVING DIR /bat/
		Inspected 20 files
		  2 files were missing
		    2 of 2 missing files were replaced
		    0 of 2 missing files were not replaced
		  2 files did not match
		    2 of 2 files were fixed
		    0 of 2 files were not fixed
		  3 files found which should be deleted
		    3 of 3 files were deleted
		    0 of 3 files were not deleted
		Calling post-update helper scripts.
		Fix successful
	EOM
	)
	assert_regex_is_output "$expected_output"
	# tracked files
	assert_file_exists "$TARGETDIR"/foo/file_1
	assert_file_not_exists "$TARGETDIR"/bar/file_2
	assert_file_exists "$TARGETDIR"/baz/file_3
	# untracked files
	assert_file_not_exists "$TARGETDIR"/bat/untracked_file1
	assert_file_exists "$TARGETDIR"/bar/untracked_file2
	assert_file_exists "$TARGETDIR"/usr/untracked_file3

}
