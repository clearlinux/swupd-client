#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

setup_file() {

	create_test_environment "$TEST_NAME"

}

teardown_file() {

	destroy_test_environment --force "$TEST_NAME"

}

@test "API068: hashdump" {

	run sudo sh -c "$SWUPD hashdump $TARGET_DIR/etc/swupd --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		6c27df6efcd6fc401ff1bc67c970b83eef115f6473db4fb9d57e5de317eba96e
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=1
