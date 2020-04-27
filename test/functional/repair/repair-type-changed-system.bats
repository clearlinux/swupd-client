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

	sudo rm -rf "$TARGETDIR"/file_*
	sudo rm -rf "$TARGETDIR"/dir_*
	sudo rm -rf "$TARGETDIR"/symlink_*
	sudo mkdir "$TARGETDIR"/other
	sudo touch "$TARGETDIR"/other/my_file

	sudo ln -s "$TARGETDIR"/other "$TARGETDIR"/dir_to_symlink_dir
	sudo ln -s "$TARGETDIR"/other/my_file "$TARGETDIR"/dir_to_symlink
	sudo ln -s broken "$TARGETDIR"/dir_to_symlink_broken
	sudo touch "$TARGETDIR"/dir_to_file

	sudo mkdir -p "$TARGETDIR"/file_to_dir/dir
	sudo touch "$TARGETDIR"/file_to_dir/dir/file
	sudo ln -s "$TARGETDIR"/other "$TARGETDIR"/file_to_symlink_dir
	sudo ln -s "$TARGETDIR"/other/my_file "$TARGETDIR"/file_to_symlink
	sudo ln -s broken "$TARGETDIR"/file_to_symlink_broken

	sudo touch "$TARGETDIR"/symlink_to_file
	sudo mkdir -p "$TARGETDIR"/symlink_to_dir/dir/
	sudo touch "$TARGETDIR"/symlink_to_dir/dir/file
	sudo ln -s "$TARGETDIR"/other "$TARGETDIR"/symlink_to_symlink

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --quiet"
	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		$PATH_PREFIX/dir_removed -> fixed
		$PATH_PREFIX/dir_to_file/test -> fixed
		$PATH_PREFIX/dir_to_symlink/test -> fixed
		$PATH_PREFIX/dir_to_symlink_broken/test -> fixed
		$PATH_PREFIX/dir_to_symlink_dir/test -> fixed
		$PATH_PREFIX/file_removed -> fixed
		$PATH_PREFIX/file_to_dir -> fixed
		$PATH_PREFIX/file_to_symlink -> fixed
		$PATH_PREFIX/file_to_symlink_broken -> fixed
		$PATH_PREFIX/file_to_symlink_dir -> fixed
		$PATH_PREFIX/symlink_to_dir -> fixed
		$PATH_PREFIX/symlink_to_file -> fixed
		$PATH_PREFIX/symlink_to_symlink -> fixed
	EOM
	)
	assert_is_output "$expected_output"

	assert_dir_exists "$TARGETDIR"/other
	assert_regular_file_exists "$TARGETDIR"/other/my_file

	assert_dir_exists "$TARGETDIR"/dir_removed
	assert_dir_exists "$TARGETDIR"/dir_to_file
	assert_dir_exists "$TARGETDIR"/dir_to_symlink
	assert_dir_exists "$TARGETDIR"/dir_to_symlink_dir
	assert_dir_exists "$TARGETDIR"/dir_to_symlink_broken

	assert_regular_file_exists "$TARGETDIR"/file_removed
	assert_regular_file_exists "$TARGETDIR"/file_to_dir
	assert_regular_file_exists "$TARGETDIR"/file_to_symlink
	assert_regular_file_exists "$TARGETDIR"/file_to_symlink_dir
	assert_regular_file_exists "$TARGETDIR"/file_to_symlink_broken

	assert_symlink_exists "$TARGETDIR"/symlink_to_file
	run stat --printf  "%N" "$TARGETDIR"/symlink_to_file
	assert_is_output "'$TARGETDIR/symlink_to_file' -> 'broken'"

	assert_symlink_exists "$TARGETDIR"/symlink_to_dir
	run stat --printf  "%N" "$TARGETDIR"/symlink_to_dir
	assert_is_output "'$TARGETDIR/symlink_to_dir' -> 'broken'"

	assert_symlink_exists "$TARGETDIR"/symlink_to_symlink
	run stat --printf  "%N" "$TARGETDIR"/symlink_to_symlink
	assert_is_output "'$TARGETDIR/symlink_to_symlink' -> 'broken'"
}
#WEIGHT=6
