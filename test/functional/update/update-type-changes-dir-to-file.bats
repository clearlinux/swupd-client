#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"

	# create a directory with some content that will
	# be used to create our test bundle
	sudo mkdir -p "$TEST_NAME"/tmp/dir1
	write_to_protected_file "$TEST_NAME"/tmp/file1 "$(generate_random_content)"
	write_to_protected_file "$TEST_NAME"/tmp/common_file "$(generate_random_content)"
	print "Content directory in version 10:\n\n $(tree "$TEST_NAME"/tmp)"

	# create the bundle using the content from tmp/
	create_bundle_from_content -t -n test-bundle1 -c "$TEST_NAME"/tmp "$TEST_NAME"

	# create a new version
	create_version "$TEST_NAME" 20 10

	# make different updates in the content
	sudo rm -rf "$TEST_NAME"/tmp/dir1
	write_to_protected_file "$TEST_NAME"/tmp/dir1 "$(generate_random_content)" # dir1: dir -> file
	sudo rm "$TEST_NAME"/tmp/file1
	sudo mkdir -p "$TEST_NAME"/tmp/file1 # file1: file -> dir
	write_to_protected_file -a "$TEST_NAME"/tmp/common_file "$(generate_random_content 1 20)" # common_file: updated
	print "\nContent directory in version 20:\n\n $(tree "$TEST_NAME"/tmp)"

	# create the update for the bundle using the updated content
	update_bundle_from_content -n test-bundle1 -c "$TEST_NAME"/tmp "$TEST_NAME"

}

@test "UPD068: Updating a system where a bundle has files that change to directories and directories that changed to files" {

	# updates should work properly regardless of file type changes
	# this test covers the type change directory -> file
	# this test covers the type change file -> directory

	assert_file_exists "$TARGET_DIR"/file1
	assert_file_exists "$TARGET_DIR"/common_file
	assert_dir_exists "$TARGET_DIR"/dir1
	sudo mkdir "$TARGET_DIR"/dir1/dir2
	show_target

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
		    new files         : 0
		    deleted files     : 0
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

	assert_dir_exists "$TARGET_DIR"/file1
	assert_dir_not_exists "$TARGET_DIR"/dir1
	assert_file_exists "$TARGET_DIR"/dir1
	assert_dir_exists "$TARGET_DIR"/.deleted.*.dir1/dir2
	assert_file_exists "$TARGET_DIR"/common_file
	show_target

}
#WEIGHT=5
