#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n bundle1 -f /file1,/file2 "$TEST_NAME"

}

@test "ADD030: Add a bundle showing detailed installation time" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS bundle1 -t"

	assert_status_is 0

	expected_output=$(cat <<-EOM
		Loading required manifests...
		Starting download of remaining update content. This may take a while...
		Installing bundle\\(s\\) files...
		Calling post-update helper scripts
		Successfully installed 1 bundle
		Raw elapsed time stats:
		.* ms: Total execution time
		.* ms: |-- Prepend bundles to list
		.* ms: |-- Install bundles
		.* ms:     |-- Add bundles and recurse
		.* ms:     |-- Consolidate files from bundles
		.* ms:     |-- Check disk space availability
		.* ms:     |-- Download packs
		.* ms:     |-- Download missing files
		.* ms:     |-- Installing bundle\\(s\\) files onto filesystem
		.* ms:     |-- Run Scripts
		CPU process time stats:
		.* ms: Total execution time
		.* ms: |-- Prepend bundles to list
		.* ms: |-- Install bundles
		.* ms:     |-- Add bundles and recurse
		.* ms:     |-- Consolidate files from bundles
		.* ms:     |-- Check disk space availability
		.* ms:     |-- Download packs
		.* ms:     |-- Download missing files
		.* ms:     |-- Installing bundle\\(s\\) files onto filesystem
		.* ms:     |-- Run Scripts
	EOM
	)

	assert_regex_is_output "$expected_output"

}
