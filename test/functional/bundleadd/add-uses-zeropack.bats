#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	test_files=/file1,/file2,/file3,/file4,/file5,/file6,/file7,/file8,/file9,/file10,/file11
	create_bundle -n test-bundle1 -f "$test_files" "$TEST_NAME"

}

@test "bundle-add uses zero packs for bundles with more than ten files" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1"

	assert_status_is 0
	# validate files are installed in target
	assert_file_exists "$TARGETDIR"/file1
	assert_file_exists "$TARGETDIR"/file2
	assert_file_exists "$TARGETDIR"/file3
	assert_file_exists "$TARGETDIR"/file4
	assert_file_exists "$TARGETDIR"/file5
	assert_file_exists "$TARGETDIR"/file6
	assert_file_exists "$TARGETDIR"/file7
	assert_file_exists "$TARGETDIR"/file8
	assert_file_exists "$TARGETDIR"/file9
	assert_file_exists "$TARGETDIR"/file10
	assert_file_exists "$TARGETDIR"/file11
	# validate zero pack was downloaded
	assert_file_exists "$STATEDIR"/pack-test-bundle1-from-0-to-10.tar
	expected_output=$(cat <<-EOM
		Downloading packs...
		Extracting test-bundle1 pack for version 10
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Installing bundle(s) files...
		Calling post-update helper scripts.
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

}
