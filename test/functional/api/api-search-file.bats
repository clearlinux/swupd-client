#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /common,/foo/file1,/foo/file3,/usr/bin/test-bin,/usr/lib/test-lib32 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /common,/bar/file2,/bar/file3,/usr/lib64/test-lib64 "$TEST_NAME"

}

@test "API050: search-file BUNDLE" {

	run sudo sh -c "$SWUPD search-file $SWUPD_OPTS test-bundle1 --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		[test-bundle1]
		/usr/share/clear/bundles/test-bundle1
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API051: search-file FILE" {

	run sudo sh -c "$SWUPD search-file $SWUPD_OPTS common --quiet"

	assert_status_is "$SWUPD_OK"
	 expected_output=$(cat <<-EOM
		[test-bundle1]
		/common
		[test-bundle2]
		/common
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API052: search-file FILE --library --binary" {

	run sudo sh -c "$SWUPD search-file $SWUPD_OPTS test- --library --binary --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		[test-bundle1]
		/usr/lib/test-lib32
		/usr/bin/test-bin
		[test-bundle2]
		/usr/lib64/test-lib64
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=4
