#!/usr/bin/env bats

load "../testlib"

export removed_file=""

test_setup() {

	create_test_environment "$TEST_NAME"
	test_files=/file1,/file2,/file3,/file4,/file5,/file6,/file7,/file8,/file9,/file10,/file11,file12
	create_bundle -n test-bundle1 -f "$test_files" "$TEST_NAME"

}

@test "ADD021: When adding a bundle and a file is missing in the pack, it falls back to using fullfiles" {

	# ----- extra setup -----
	# replace the original zero pack with one with one file missing
	sudo mkdir "$TEST_NAME"/temp
	sudo tar -C "$TEST_NAME"/temp -xf "$WEBDIR"/10/pack-test-bundle1-from-0.tar
	removed_file="$(find "$TEST_NAME"/temp/staged -type f -printf '%f' -quit)"
	sudo rm "$TEST_NAME"/temp/staged/"$removed_file"
	sudo rm "$WEBDIR"/10/pack-test-bundle1-from-0.tar
	sudo tar -C "$TEST_NAME"/temp -cf "$WEBDIR"/10/pack-test-bundle1-from-0.tar staged
	# -----------------------

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
	assert_file_exists "$TARGETDIR"/file12
	# validate zero pack was downloaded
	assert_file_exists "$STATEDIR"/pack-test-bundle1-from-0-to-10.tar
	# validate the file missing from the zero pack was downloaded
	assert_file_exists "$STATEDIR"/staged/"$removed_file"
	expected_output=$(cat <<-EOM
		Loading required manifests...
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Starting download of remaining update content. This may take a while...
		Installing files...
		Calling post-update helper scripts
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

}

@test "ADD031: When adding a bundle and the zero pack is missing, it falls back to using fullfiles" {

	# ----- extra setup -----
	# remove the original zero pack
	sudo rm "$WEBDIR"/10/pack-test-bundle1-from-0.tar
	# -----------------------

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
	assert_file_exists "$TARGETDIR"/file12
	expected_output=$(cat <<-EOM
		Loading required manifests...
		Downloading packs for:
		 - test-bundle1
		Finishing packs extraction...
		Starting download of remaining update content. This may take a while...
		Installing files...
		Calling post-update helper scripts
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

}
