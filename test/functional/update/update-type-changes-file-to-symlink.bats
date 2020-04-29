#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"

	# create a directory with some content that will
	# be used to create our test bundle
	sudo mkdir -p "$TEST_NAME"/tmp/dir1/dir2
	write_to_protected_file "$TEST_NAME"/tmp/file1 "$(generate_random_content)"
	write_to_protected_file "$TEST_NAME"/tmp/dir1/dir2/file2 "$(generate_random_content)"
	write_to_protected_file "$TEST_NAME"/tmp/file3 "$(generate_random_content)"
	print "Content directory in version 10:\n\n $(tree "$TEST_NAME"/tmp)"

	# create the bundle using the content from tmp/
	create_bundle_from_content -t -n test-bundle1 -c "$TEST_NAME"/tmp "$TEST_NAME"

	# create a new version
	create_version "$TEST_NAME" 20 10

	# make different updates in the content
	sudo mv "$TEST_NAME"/tmp/file1 "$TEST_NAME"/tmp/new_file1
	sudo ln -s --relative "$TEST_NAME"/tmp/new_file1 "$TEST_NAME"/tmp/file1 # file1: file -> symlink
	sudo mv "$TEST_NAME"/tmp/dir1/dir2 "$TEST_NAME"/tmp/dir1/new_dir2
	sudo ln -s --relative "$TEST_NAME"/tmp/dir1/new_dir2 "$TEST_NAME"/tmp/dir1/dir2 # dir2: dir -> symlink
	write_to_protected_file -a "$TEST_NAME"/tmp/file3 "$(generate_random_content 1 20)" # file3: updated
	write_to_protected_file "$TEST_NAME"/tmp/file4 "$(generate_random_content)" # file4: new file
	print "\nContent directory in version 20:\n\n $(tree "$TEST_NAME"/tmp)"

	# create the update for the bundle using the updated content
	update_bundle_from_content -n test-bundle1 -c "$TEST_NAME"/tmp "$TEST_NAME"

}

@test "UPD066: Updating a system where a bundle has files that changed to symlinks" {

	# updates should work properly regardless of file type changes
	# this test covers the type change file -> symlink
	# this test covers the type change directory -> symlink

	assert_file_exists "$TARGETDIR"/file1
	assert_dir_exists "$TARGETDIR"/dir1/dir2
	assert_file_exists "$TARGETDIR"/dir1/dir2/file2
	assert_file_exists "$TARGETDIR"/file3

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
		    changed files     : 3
		    new files         : 4
		    deleted files     : 1
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Update was applied
		Calling post-update helper scripts
		2 files were not in a pack
		Update successful - System updated from version 10 to version 20
	EOM
	)
	assert_is_output "$expected_output"

	assert_symlink_exists "$TARGETDIR"/file1
	assert_symlink_exists "$TARGETDIR"/dir1/dir2
	assert_file_exists "$TARGETDIR"/dir1/dir2/file2
	assert_file_exists "$TARGETDIR"/file3
	assert_file_exists "$TARGETDIR"/file4
	show_target

}
#WEIGHT=6
