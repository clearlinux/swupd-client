#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"

	write_to_protected_file "$TEST_NAME"/external_file "$(generate_random_content)"
	# create a directory with some content that will
	# be used to create our test bundle
	sudo mkdir -p "$TEST_NAME"/tmp/dir1/dirA
	write_to_protected_file "$TEST_NAME"/tmp/fileA "$(generate_random_content)"
	sudo ln -s --relative "$TEST_NAME"/tmp/fileA "$TEST_NAME"/tmp/file1
	sudo ln -s --relative "$TEST_NAME"/tmp/dir1/dirA "$TEST_NAME"/tmp/dir1/dir2
	write_to_protected_file "$TEST_NAME"/tmp/dir1/dir2/file2 "$(generate_random_content)"
	write_to_protected_file "$TEST_NAME"/tmp/file3 "$(generate_random_content)"
	sudo ln -s "$TEST_NAME"/external_file "$TEST_NAME"/tmp/external_file

	# create the bundle using the content from tmp/
	create_bundle_from_content -t -n test-bundle1 -c "$TEST_NAME"/tmp "$TEST_NAME"

	# create a new version
	create_version "$TEST_NAME" 20 10

	# make different updates in the content
	# change symlink /file1 to be a file
	sudo rm "$TEST_NAME"/tmp/file1
	sudo mv "$TEST_NAME"/tmp/fileA "$TEST_NAME"/tmp/file1
	# change symlink /dir2 to be a dir
	sudo rm "$TEST_NAME"/tmp/dir1/dir2
	sudo mv "$TEST_NAME"/tmp/dir1/dirA "$TEST_NAME"/tmp/dir1/dir2
	# change absolute symlink /external_file to be a file
	sudo rm "$TEST_NAME"/tmp/external_file
	sudo mv "$TEST_NAME"/external_file "$TEST_NAME"/tmp/external_file

	# update file3
	write_to_protected_file -a "$TEST_NAME"/tmp/file3 "$(generate_random_content 1 20)"
	# add a new file to the bundle
	write_to_protected_file "$TEST_NAME"/tmp/file4 "$(generate_random_content)"

	# create the update for the bundle using the updated content
	update_bundle_from_content -n test-bundle1 -c "$TEST_NAME"/tmp "$TEST_NAME"

}

@test "UPD067: Updating a system where a bundle has symlinks that changed to files" {

	# updates should work properly regardless of file type changes
	# this test covers the type change relative symlink -> file
	# this test covers the type change absolute symlink -> file
	# this test covers the type change symlink -> directory

	assert_file_exists "$TARGETDIR"/fileA
	assert_symlink_exists "$TARGETDIR"/file1
	assert_dir_exists "$TARGETDIR"/dir1/dirA
	assert_symlink_exists "$TARGETDIR"/dir1/dir2
	assert_symlink_exists "$TARGETDIR"/external_file

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 20
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 4
		    new files         : 2
		    deleted files     : 3
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Update was applied
		Calling post-update helper scripts
		3 files were not in a pack
		Update successful - System updated from version 10 to version 20
	EOM
	)
	assert_is_output "$expected_output"

	assert_symlink_not_exists "$TARGETDIR"/file1
	assert_file_exists "$TARGETDIR"/file1
	assert_symlink_not_exists "$TARGETDIR"/dir1/dir2
	assert_dir_exists "$TARGETDIR"/dir1/dir2
	assert_symlink_not_exists "$TARGETDIR"/external_file
	assert_file_exists "$TARGETDIR"/external_file
	assert_file_exists "$TARGETDIR"/dir1/dir2/file2
	assert_file_exists "$TARGETDIR"/file3
	assert_file_exists "$TARGETDIR"/file4

}
