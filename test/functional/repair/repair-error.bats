#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment -r "$TEST_NAME" 10 1
	versionurl_hash=$(sudo "$SWUPD" hashdump --quiet "$TARGETDIR"/usr/share/defaults/swupd/versionurl)
	sudo cp "$TARGETDIR"/usr/share/defaults/swupd/versionurl "$WEBDIR"/10/files/"$versionurl_hash"
	versionurl="$WEBDIR"/10/files/"$versionurl_hash"
	contenturl_hash=$(sudo "$SWUPD" hashdump --quiet "$TARGETDIR"/usr/share/defaults/swupd/contenturl)
	sudo cp "$TARGETDIR"/usr/share/defaults/swupd/contenturl "$WEBDIR"/10/files/"$contenturl_hash"
	contenturl="$WEBDIR"/10/files/"$contenturl_hash"
	create_bundle -L -n os-core-update -f /usr/share/defaults/swupd/versionurl:"$versionurl",/usr/share/defaults/swupd/contenturl:"$contenturl" "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /foo/file_1,/bar/file_2 "$TEST_NAME"
	create_version "$TEST_NAME" 20 10 1
	update_bundle -p "$TEST_NAME" test-bundle1 --update /foo/file_1
	update_bundle -p "$TEST_NAME" test-bundle1 --delete /bar/file_2
	update_bundle "$TEST_NAME" test-bundle1 --add /baz/bat/file_3

	# force some repairs in the target system
	set_current_version "$TEST_NAME" 20
	sudo touch "$TARGETDIR"/usr/untracked_file

	# force failures while repairing
	sudo touch "$TARGETDIR"/baz
	sudo chattr +i "$TARGETDIR"/baz
	sudo chattr +i "$TARGETDIR"/usr/untracked_file

}

test_teardown() {

	sudo chattr -i "$TARGETDIR"/usr/untracked_file
	sudo chattr -i "$TARGETDIR"/baz

}

@test "REP045: Repair failures" {

	# Make sure the output is useful when repair fails

	run sudo sh -c "$SWUPD repair $SWUPD_OPTS --picky"

	assert_status_is_not "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Diagnosing version 20
		Downloading missing manifests...
		Checking for corrupt files
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Adding any missing files
		Error: Target exists but is not a directory: $PATH_PREFIX/baz
		 -> Missing file: $PATH_PREFIX/baz/bat -> not fixed
		Error: Target has different file type but could not be removed: $PATH_PREFIX/baz
		Error: Target directory does not exist and could not be created: $PATH_PREFIX/baz/bat
		 -> Missing file: $PATH_PREFIX/baz/bat/file_3 -> not fixed
		Repairing corrupt files
		Error: Target has different file type but could not be removed: $PATH_PREFIX/baz
		 -> Hash mismatch for file: $PATH_PREFIX/baz -> not fixed
		 -> Hash mismatch for file: $PATH_PREFIX/foo/file_1 -> fixed
		 -> Hash mismatch for file: $PATH_PREFIX/usr/lib/os-release -> fixed
		The removal of extraneous files will be skipped due to the previous errors found repairing
		Removing extra files under $PATH_PREFIX/usr
		 -> Extra file: $PATH_PREFIX/usr/untracked_file -> not deleted (Operation not permitted)
		Inspected 23 files
		  2 files were missing
		    0 of 2 missing files were replaced
		    2 of 2 missing files were not replaced
		  3 files did not match
		    2 of 3 files were repaired
		    1 of 3 files were not repaired
		  1 file found which should be deleted
		    0 of 1 files were deleted
		    1 of 1 files were not deleted
		Calling post-update helper scripts
		Repair did not fully succeed
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=6
