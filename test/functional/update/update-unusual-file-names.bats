#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_version -p "$TEST_NAME" 20 10

	update_bundle "$TEST_NAME" os-core --add "/usr/simple_file"
	file=$(get_hash_from_manifest "$TEST_NAME"/web-dir/20/Manifest.os-core /usr/simple_file)

	# Unusual directory names
	update_bundle "$TEST_NAME" os-core --add "/usr/unusual_'dirname'/normal_file1"
	update_bundle "$TEST_NAME" os-core --add '/usr/unusual_"dirname"/normal_file2'
	update_bundle "$TEST_NAME" os-core --add '/usr/very_unusual_,¹²³!@#$%^&*()_;[]{}\`_name/normal_file3'
	update_bundle "$TEST_NAME" os-core --add "/usr/unusual:dirname/normal_file4:""$TEST_NAME"/web-dir/20/files/"$file"

	# Unusual file names
	update_bundle "$TEST_NAME" os-core --add "/usr/unusual_files/'file1'"
	update_bundle "$TEST_NAME" os-core --add '/usr/unusual_files/"file2"'
	update_bundle "$TEST_NAME" os-core --add '/usr/unusual_files/very_unusual_,¹²³!@#$%^&*()_;[]{}\_file3'
	update_bundle "$TEST_NAME" os-core --add "/usr/unusual_files/file:4:""$TEST_NAME"/web-dir/20/files/"$file"

	# Unusual link names
	create_bundle -n test-bundle1 -l "/usr/links/'link1'",'/usr/links/"link2"','/usr/links/very_unusual_¹²³!@#$%^&*()_;[]{}\_link3','/usr/links/link:4' "$TEST_NAME"
	add_dependency_to_manifest "$WEB_DIR"/20/Manifest.os-core test-bundle1

}

@test "UPD071: Unusual file name" {

	# Check if installation of files with names using unusual characters
	# are all successful.
	run sudo sh -c "$SWUPD update $SWUPD_OPTS"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 20
		Downloading packs for:
		 - test-bundle1
		 - os-core
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 1
		    new bundles       : 1
		    deleted bundles   : 0
		    changed files     : 0
		    new files         : 25
		    deleted files     : 0
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Update was applied
		Calling post-update helper scripts
		4 files were not in a pack
		Update successful - System updated from version 10 to version 20
	EOM
	)
	assert_is_output "$expected_output"

	assert_file_exists "$TARGET_DIR""/usr/simple_file"

	# Unusual dir names
	assert_file_exists "$TARGET_DIR""/usr/unusual_'dirname'/normal_file1"
	assert_file_exists "$TARGET_DIR"'/usr/unusual_"dirname"/normal_file2'
	assert_file_exists "$TARGET_DIR"'/usr/very_unusual_,¹²³!@#$%^&*()_;[]{}\`_name/normal_file3'
	assert_file_exists "$TARGET_DIR"'/usr/unusual:dirname/normal_file4'

	# Unusual file names
	assert_file_exists "$TARGET_DIR""/usr/unusual_files/'file1'"
	assert_file_exists "$TARGET_DIR"'/usr/unusual_files/"file2"'
	assert_file_exists "$TARGET_DIR"'/usr/unusual_files/very_unusual_,¹²³!@#$%^&*()_;[]{}\_file3'
	assert_file_exists "$TARGET_DIR""/usr/unusual_files/file:4"

	# Unusual link names
	assert_symlink_exists "$TARGET_DIR""/usr/links/'link1'"
	assert_symlink_exists "$TARGET_DIR"'/usr/links/"link2"'
	assert_symlink_exists "$TARGET_DIR"'/usr/links/very_unusual_¹²³!@#$%^&*()_;[]{}\_link3'
	assert_symlink_exists "$TARGET_DIR""/usr/links/link:4"
}
#WEIGHT=10
