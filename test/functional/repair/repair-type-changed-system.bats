#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"

	# Generate dir
	sudo mkdir -p "$TEST_NAME"/tmp/dir_removed
	sudo mkdir -p "$TEST_NAME"/tmp/dir_to_file
	sudo mkdir -p "$TEST_NAME"/tmp/dir_to_symlink
	sudo mkdir -p "$TEST_NAME"/tmp/dir_to_symlink_dir
	sudo mkdir -p "$TEST_NAME"/tmp/dir_to_symlink_broken
	write_to_protected_file "$TEST_NAME"/tmp/dir_to_file/test "$(generate_random_content)"
	write_to_protected_file "$TEST_NAME"/tmp/dir_to_symlink/test "$(generate_random_content)"
	write_to_protected_file "$TEST_NAME"/tmp/dir_to_symlink_dir/test "$(generate_random_content)"
	write_to_protected_file "$TEST_NAME"/tmp/dir_to_symlink_broken/test "$(generate_random_content)"

	# Generate files
	write_to_protected_file "$TEST_NAME"/tmp/file_removed "$(generate_random_content)"
	write_to_protected_file "$TEST_NAME"/tmp/file_to_dir "$(generate_random_content)"
	write_to_protected_file "$TEST_NAME"/tmp/file_to_symlink "$(generate_random_content)"
	write_to_protected_file "$TEST_NAME"/tmp/file_to_symlink_dir "$(generate_random_content)"
	write_to_protected_file "$TEST_NAME"/tmp/file_to_symlink_broken "$(generate_random_content)"

	# Generate symlinks
	sudo ln -s broken "$TEST_NAME"/tmp/symlink_to_file
	sudo ln -s broken "$TEST_NAME"/tmp/symlink_to_dir
	sudo ln -s broken "$TEST_NAME"/tmp/symlink_to_symlink

	# create the bundle using the content from tmp/
	create_bundle_from_content -t -n test-bundle1 -c "$TEST_NAME"/tmp "$TEST_NAME"
}

@test "REP047: Repair should be able to repair files that have changed in the system" {

	# Corrupted files could have a type different from the original
	# and repair should be able to repair them.

	sudo rm -rf "$TARGET_DIR"/file_*
	sudo rm -rf "$TARGET_DIR"/dir_*
	sudo rm -rf "$TARGET_DIR"/symlink_*
	sudo mkdir "$TARGET_DIR"/other
	sudo touch "$TARGET_DIR"/other/my_file

	sudo ln -s "$TARGET_DIR"/other "$TARGET_DIR"/dir_to_symlink_dir
	sudo ln -s "$TARGET_DIR"/other/my_file "$TARGET_DIR"/dir_to_symlink
	sudo ln -s broken "$TARGET_DIR"/dir_to_symlink_broken
	sudo touch "$TARGET_DIR"/dir_to_file

	sudo mkdir -p "$TARGET_DIR"/file_to_dir/dir
	sudo touch "$TARGET_DIR"/file_to_dir/dir/file
	sudo ln -s "$TARGET_DIR"/other "$TARGET_DIR"/file_to_symlink_dir
	sudo ln -s "$TARGET_DIR"/other/my_file "$TARGET_DIR"/file_to_symlink
	sudo ln -s broken "$TARGET_DIR"/file_to_symlink_broken

	sudo touch "$TARGET_DIR"/symlink_to_file
	sudo mkdir -p "$TARGET_DIR"/symlink_to_dir/dir/
	sudo touch "$TARGET_DIR"/symlink_to_dir/dir/file
	sudo ln -s "$TARGET_DIR"/other "$TARGET_DIR"/symlink_to_symlink

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --quiet"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		$ABS_TARGET_DIR/dir_removed -> fixed
		$ABS_TARGET_DIR/dir_to_file/test -> fixed
		$ABS_TARGET_DIR/dir_to_symlink/test -> fixed
		$ABS_TARGET_DIR/dir_to_symlink_broken/test -> fixed
		$ABS_TARGET_DIR/dir_to_symlink_dir/test -> fixed
		$ABS_TARGET_DIR/file_removed -> fixed
		$ABS_TARGET_DIR/file_to_dir -> fixed
		$ABS_TARGET_DIR/file_to_symlink -> fixed
		$ABS_TARGET_DIR/file_to_symlink_broken -> fixed
		$ABS_TARGET_DIR/file_to_symlink_dir -> fixed
		$ABS_TARGET_DIR/symlink_to_dir -> fixed
		$ABS_TARGET_DIR/symlink_to_file -> fixed
		$ABS_TARGET_DIR/symlink_to_symlink -> fixed
	EOM
	)
	assert_is_output "$expected_output"

	assert_dir_exists "$TARGET_DIR"/other
	assert_regular_file_exists "$TARGET_DIR"/other/my_file

	assert_dir_exists "$TARGET_DIR"/dir_removed
	assert_dir_exists "$TARGET_DIR"/dir_to_file
	assert_dir_exists "$TARGET_DIR"/dir_to_symlink
	assert_dir_exists "$TARGET_DIR"/dir_to_symlink_dir
	assert_dir_exists "$TARGET_DIR"/dir_to_symlink_broken

	assert_regular_file_exists "$TARGET_DIR"/file_removed
	assert_regular_file_exists "$TARGET_DIR"/file_to_dir
	assert_regular_file_exists "$TARGET_DIR"/file_to_symlink
	assert_regular_file_exists "$TARGET_DIR"/file_to_symlink_dir
	assert_regular_file_exists "$TARGET_DIR"/file_to_symlink_broken

	assert_symlink_exists "$TARGET_DIR"/symlink_to_file
	run stat --printf  "%N" "$TARGET_DIR"/symlink_to_file
	assert_is_output "'$TARGET_DIR/symlink_to_file' -> 'broken'"

	assert_symlink_exists "$TARGET_DIR"/symlink_to_dir
	run stat --printf  "%N" "$TARGET_DIR"/symlink_to_dir
	assert_is_output "'$TARGET_DIR/symlink_to_dir' -> 'broken'"

	assert_symlink_exists "$TARGET_DIR"/symlink_to_symlink
	run stat --printf  "%N" "$TARGET_DIR"/symlink_to_symlink
	assert_is_output "'$TARGET_DIR/symlink_to_symlink' -> 'broken'"
}
#WEIGHT=6
