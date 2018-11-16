#!/usr/bin/env bats

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"
	printf "test-data" | sudo tee "$TARGETDIR"/test-hash > /dev/null

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

@test "HSD001: Calculate the hash of a file" {

	run sudo sh -c "$SWUPD hashdump $TARGETDIR/test-hash"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Calculating hash with xattrs for: .*/target-dir/test-hash
		8286279c93f45c7ffa6b9ed440066de09716527346d9dd0239f50948e0e554f0
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "HSD002: Try calculating the hash of a file that does not exist" {

	# attempting to calculate the hash from a non existent file should still
	# succeed but should return a hash of all zeros

	run sudo sh -c "$SWUPD hashdump $TARGETDIR/fake-file"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Calculating hash with xattrs for: .*/target-dir/fake-file
		0000000000000000000000000000000000000000000000000000000000000000
	EOM
	)
	assert_regex_is_output "$expected_output"

}

@test "HSD003: Calculate the hash of a file specifying its path separatelly" {

	run sudo sh -c "$SWUPD hashdump --path=$TARGETDIR test-hash"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Calculating hash with xattrs for: .*/target-dir/test-hash
		8286279c93f45c7ffa6b9ed440066de09716527346d9dd0239f50948e0e554f0
	EOM
	)
	assert_regex_is_output "$expected_output"

}
