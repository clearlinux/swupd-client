#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	test_files=/usr/bin/1,/usr/bin/2,/usr/bin/3,/usr/bin/4,/usr/bin/5,/usr/bin/6,/usr/bin/7,/usr/bin/8,/usr/bin/9,/usr/bin/10
	create_bundle -n test-bundle1 -f "$test_files" "$TEST_NAME"
	create_bundle -n test-bundle2 -f /media/lib/file2 "$TEST_NAME"

}

@test "ADD020: Adding multiple bundles using packs" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1 test-bundle2"

	assert_status_is 0
	assert_dir_exists "$TARGETDIR/usr/bin"
	assert_dir_exists "$TARGETDIR/media/lib"
	assert_file_exists "$TARGETDIR/usr/bin/10"
	assert_file_exists "$TARGETDIR/media/lib/file2"
	expected_output=$(cat <<-EOM
		Loading required manifests...
		Downloading packs for:
		 - test-bundle1
		 - test-bundle2
		Finishing packs extraction...
		No extra files need to be downloaded
		Installing bundle(s) files...
		Calling post-update helper scripts
		Successfully installed 2 bundles
	EOM
	)
	assert_is_output "$expected_output"

}
