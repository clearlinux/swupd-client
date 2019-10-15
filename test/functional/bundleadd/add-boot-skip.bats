#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	# create a bundle with a boot file (in /usr/lib/kernel)
	create_bundle -n test-bundle -f /usr/lib/kernel/test-file "$TEST_NAME"

}

@test "ADD028: Adding a bundle containing a boot file without updating the boot files" {

	run sudo sh -c "$SWUPD bundle-add -b $SWUPD_OPTS test-bundle"

	assert_status_is 0
	assert_file_exists "$TARGETDIR/usr/lib/kernel/test-file"
	expected_output=$(cat <<-EOM
		Loading required manifests...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Calling post-update helper scripts
		Warning: boot files update skipped due to --no-boot-update argument
		Successfully installed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"

}
