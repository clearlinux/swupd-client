#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

global_setup() {

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
	update_bundle "$TEST_NAME" test-bundle1 --add /baz/file_3

}

@test "API053: diagnose (no issues found)" {

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --picky --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty

}

@test "API054: diagnose (issues found)" {

	# set the current version of the target system as if it is already
	# at version 20 so diagnose find issues
	set_current_version "$TEST_NAME" 20
	# adding an untracked file into /usr
	sudo touch "$TARGETDIR"/usr/untracked_file3

	run sudo sh -c "$SWUPD diagnose $SWUPD_OPTS --picky --quiet"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		$PATH_PREFIX/baz
		$PATH_PREFIX/baz/file_3
		$PATH_PREFIX/foo/file_1
		$PATH_PREFIX/usr/lib/os-release
		$PATH_PREFIX/bar/file_2
		$PATH_PREFIX/usr/untracked_file3
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=6
