#!/usr/bin/env bats

load "../testlib"

setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle -f /usr/bin/test-file "$TEST_NAME"
	# set up state directory with bad hash file and pack hint
	file_hash=$(get_hash_from_manifest "$TEST_NAME"/web-dir/10/Manifest.test-bundle /usr/bin/test-file)
	sudo mkdir -p "$TEST_NAME"/state/staged
	sudo sh -c "echo \"test file MODIFIED\" > $TEST_NAME/state/staged/$file_hash"
	sudo touch "$TEST_NAME"/state/pack-test-bundle-from-0-to-10.tar

}

teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "bundle-add add bundle with bad hash in state dir" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

	assert_status_is 0
	assert_file_exists "$TEST_NAME"/target-dir/usr/bin/test-file
	expected_output=$(cat <<-EOM
		Starting download of remaining update content. This may take a while...

		File /usr/bin/test-file was not in a pack
		.
		Finishing download of update content...
		Installing bundle(s) files...
		.
		Calling post-update helper scripts.
		Successfully installed 1 bundle
	EOM
	)
	assert_in_output "$expected_output"

}
