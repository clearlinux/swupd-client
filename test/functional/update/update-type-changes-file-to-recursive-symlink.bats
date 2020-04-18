#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME"
	CONTENT="$TEST_NAME"/content_dir

	# content for bundle test-bundle1
	sudo mkdir -p "$CONTENT"/32/bits
	sudo mkdir -p "$CONTENT"/32/ext
	sudo mkdir "$CONTENT"/bits
	sudo mkdir "$CONTENT"/ext
	write_to_protected_file "$CONTENT"/bits/f1 "$(generate_random_content)"
	write_to_protected_file "$CONTENT"/bits/f2 "$(generate_random_content)"
	write_to_protected_file "$CONTENT"/bits/f3 "$(generate_random_content)"
	write_to_protected_file "$CONTENT"/ext/f4 "$(generate_random_content)"
	sudo cp "$CONTENT"/bits/f1 "$CONTENT"/32/bits/f1
	sudo cp "$CONTENT"/bits/f2 "$CONTENT"/32/bits/f2
	sudo cp "$CONTENT"/bits/f3 "$CONTENT"/32/bits/f3
	sudo cp "$CONTENT"/ext/f4 "$CONTENT"/32/ext/f4

	create_bundle_from_content -t -n test-bundle1 -c "$CONTENT" "$TEST_NAME"
	print "Content directory in version 10:\n\n $(tree "$CONTENT")"

	create_version -r "$TEST_NAME" 20 10

	# content for update
	sudo rm -rf "$CONTENT"/32
	sudo ln -s --relative "$CONTENT" "$CONTENT"/32

	update_bundle_from_content -n test-bundle1 -c "$CONTENT" "$TEST_NAME"
	print "Content directory in version 20:\n\n $(tree "$CONTENT")"

}

@test "UPD069: Updating a system where a bundle has a directory that changed to become a symlink to ->." {

	# if during an update, a directory is replaced by a symlink that causes recursion,
	# meaning the symlink points to the current directory where itself resides (link -> .),
	# swupd should still update correctly.
	# Reference issue https://github.com/clearlinux/swupd-client/issues/890

	assert_file_exists "$TARGETDIR"/bits/f1
	assert_file_exists "$TARGETDIR"/bits/f2
	assert_file_exists "$TARGETDIR"/bits/f3
	assert_file_exists "$TARGETDIR"/ext/f4
	assert_file_exists "$TARGETDIR"/32/bits/f1
	assert_file_exists "$TARGETDIR"/32/bits/f2
	assert_file_exists "$TARGETDIR"/32/bits/f3
	assert_file_exists "$TARGETDIR"/32/ext/f4
	assert_symlink_not_exists "$TARGETDIR"/32

	run sudo sh -c "$SWUPD update $SWUPD_OPTS"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Update started
		Preparing to update from 10 to 20
		Downloading packs for:
		 - os-core
		 - test-bundle1
		Finishing packs extraction...
		Statistics for going from version 10 to version 20:
		    changed bundles   : 2
		    new bundles       : 0
		    deleted bundles   : 0
		    changed files     : 3
		    new files         : 0
		    deleted files     : 6
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Update was applied
		Calling post-update helper scripts
		1 files were not in a pack
		Update successful - System updated from version 10 to version 20
	EOM
	)
	assert_is_output "$expected_output"

	assert_file_exists "$TARGETDIR"/bits/f1
	assert_file_exists "$TARGETDIR"/bits/f2
	assert_file_exists "$TARGETDIR"/bits/f3
	assert_file_exists "$TARGETDIR"/ext/f4
	assert_symlink_exists "$TARGETDIR"/32
	assert_file_exists "$TARGETDIR"/32/bits/f1
	assert_file_exists "$TARGETDIR"/32/bits/f2
	assert_file_exists "$TARGETDIR"/32/bits/f3
	assert_file_exists "$TARGETDIR"/32/ext/f4

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS"
	assert_status_is "$SWUPD_OK"

}
#WEIGHT=10
