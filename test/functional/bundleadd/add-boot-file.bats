#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	# create a bundle with a boot file (in /usr/lib/kernel)
	create_bundle -n test-bundle -f /usr/lib/kernel/test-file "$TEST_NAME"

}

@test "ADD014: Adding a bundle containing a boot file" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

	assert_status_is 0
	assert_file_exists "$TARGETDIR/usr/lib/kernel/test-file"
	expected_output=$(cat <<-EOM
		Starting download of remaining update content. This may take a while...
		.*...33%
		.*...66%
		.*...100%

		Finishing download of update content...
		Installing bundle\(s\) files...
		.*...16%
		.*...33%
		.*...50%
		.*...66%
		.*...83%
		.*...100%

		Calling post-update helper scripts.
		Warning: post-update helper script \($TEST_DIRNAME/testfs/target-dir//usr/bin/clr-boot-manager\) not found, it will be skipped
		Successfully installed 1 bundle
	EOM
	)
	assert_regex_is_output --identical "$expected_output"

}
