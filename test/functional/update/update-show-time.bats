#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle -f /foo "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	update_bundle "$TEST_NAME" test-bundle --update /foo

}

@test "UPD055: Update a system showing detailed operation time" {

	run sudo sh -c "$SWUPD update $SWUPD_OPTS --time"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Raw elapsed time stats:
		.* ms: Total execution time
		.* ms: |-- Prepare for update
		.* ms: |-- Get versions
		.* ms: |-- Clean up download directory
		.* ms: |-- Load manifests
		.* ms:     |-- Load MoM manifests
		.* ms:     |-- Recurse and consolidate bundle manifests
		.* ms:         |-- Add included bundle manifests
		.* ms: |-- Run pre-update scripts
		.* ms: |-- Download packs
		.* ms: |-- Apply deltas
		.* ms: |-- Create update list
		.* ms: |-- Update loop
		.* ms: |-- Run post-update scripts

	CPU process time stats:
	.* ms: Total execution time
	.* ms: |-- Prepare for update
	.* ms: |-- Get versions
	.* ms: |-- Clean up download directory
	.* ms: |-- Load manifests
	.* ms:     |-- Load MoM manifests
	.* ms:     |-- Recurse and consolidate bundle manifests
	.* ms:         |-- Add included bundle manifests
	.* ms: |-- Run pre-update scripts
	.* ms: |-- Download packs
	.* ms: |-- Apply deltas
	.* ms: |-- Create update list
	.* ms: |-- Update loop
	.* ms: |-- Run post-update scripts
	Update successful. System updated from version 10 to version 20
	EOM
	)
	assert_regex_in_output "$expected_output"

}
