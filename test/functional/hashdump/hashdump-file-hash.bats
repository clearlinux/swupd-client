#!/usr/bin/env bats

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"
	printf "test-data" | sudo tee "$TEST_NAME"/target-dir/test-hash > /dev/null

}

test_setup() {

	return

}

test_teardown() {

	return

}

global_teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "hashdump with prefix" {

	run sudo sh -c "$SWUPD hashdump --path=$TEST_NAME/target-dir /test-hash"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Calculating hash with xattrs for: .*/target-dir/test-hash
		8286279c93f45c7ffa6b9ed440066de09716527346d9dd0239f50948e0e554f0
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "hashdump with no prefix" {

	run sudo sh -c "$SWUPD hashdump $TEST_NAME/target-dir/test-hash"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Calculating hash with xattrs for: .*/target-dir/test-hash
		8286279c93f45c7ffa6b9ed440066de09716527346d9dd0239f50948e0e554f0
	EOM
	)
	assert_regex_is_output "$expected_output"

}
