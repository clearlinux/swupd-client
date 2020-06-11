#!/usr/bin/env bats

# Author: John Akre
# Email: john.w.akre@intel.com

load "../testlib"

test_setup() {

	create_test_environment -e "$TEST_NAME" 10
	create_bundle -n os-core -f /core "$TEST_NAME"

}

@test "INS015: Download os-install content without installing" {

	run sudo sh -c "$SWUPD os-install $SWUPD_OPTS_NO_PATH --download $TARGET_DIR"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Installing OS version 10 (latest)
		Downloading missing manifests...
		Downloading packs for:
		 - os-core
		Finishing packs extraction...
		Checking for corrupt files
		Validate downloaded files
		No extra files need to be downloaded
		Installation files downloaded
	EOM
	)

	assert_is_output "$expected_output"
	assert_file_not_exists "$TARGET_DIR"/core

	assert_file_exists "$ABS_MANIFEST_DIR"/10/Manifest.MoM
	assert_file_exists "$ABS_MANIFEST_DIR"/10/Manifest.os-core

	core_hash=$(get_hash_from_manifest "$ABS_MANIFEST_DIR"/10/Manifest.os-core "/core")
	assert_file_exists "$ABS_STAGED_DIR"/"$core_hash"

}
#WEIGHT=1
