#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	sudo mkdir "$ABS_MANIFEST_DIR"/10
	sudo touch "$ABS_MANIFEST_DIR"/10/Manifest.test{1..3}
	sudo touch "$ABS_CACHE_DIR"/pack-test{1..2}-from-0.tar

}

@test "API063: clean" {

	run sudo sh -c "$SWUPD clean $SWUPD_OPTS --quiet"

	assert_status_is "$SWUPD_OK"
	assert_output_is_empty

}

@test "API064: clean --dry-run" {

	run sudo sh -c "$SWUPD clean $SWUPD_OPTS --dry-run --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		$ABS_CACHE_DIR/pack-test.-from-0.tar
		$ABS_CACHE_DIR/pack-test.-from-0.tar
		$ABS_DELTA_DIR
		$ABS_DOWNLOAD_DIR
		$ABS_STAGED_DIR
		$ABS_TEMP_DIR
		$ABS_MANIFEST_DIR/10/Manifest.test.
		$ABS_MANIFEST_DIR/10/Manifest.test.
		$ABS_MANIFEST_DIR/10/Manifest.test.
	EOM
	)
	assert_regex_is_output "$expected_output"

}
#WEIGHT=2
