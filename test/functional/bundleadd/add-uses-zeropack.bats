#!/usr/bin/env bats

load "../testlib"

metadata_setup() {

	create_test_environment "$TEST_NAME"
	test_files=/file1,/file2,/file3,/file4,/file5,/file6,/file7,/file8,/file9,/file10,/file11
	create_bundle -n test-bundle1 -f "$test_files" "$TEST_NAME"

}

@test "ADD019: When adding a bundle with more than 10 files, zero packs should be used" {

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
		Loading required manifests...
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Validate downloaded files
		No extra files need to be downloaded
		Installing files...
		Calling post-update helper scripts
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=3
