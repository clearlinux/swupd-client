#!/usr/bin/env bats

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n test-bundle1 -f /foo/test-file1 "$TEST_NAME"
	create_bundle -L -n test-bundle2 -f /bar/test-file2 "$TEST_NAME"
	create_bundle -n test-bundle3 -f /baz/test-file3 "$TEST_NAME"

}

test_setup() {

	# do nothing, just overwrite the lib test_setup
	return

}

test_teardown() {

	# reinstall test-bundle1
	install_bundle "$TEST_NAME"/web-dir/10/Manifest.test-bundle1
	install_bundle "$TEST_NAME"/web-dir/10/Manifest.test-bundle2

}

global_teardown() {

	destroy_test_environment "$TEST_NAME"

}

# ------------------------------------------
# Good Cases (all good bundles)
# ------------------------------------------

@test "bundle-remove output: removing one bundle" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Deleting bundle files...
		Total deleted files: 2
		Successfully removed 1 bundle
	EOM
	)
	assert_is_output --identical "$expected_output"

}

@test "bundle-remove output: removing multiple bundles" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1 test-bundle2"
	assert_status_is 0
	expected_output=$(cat <<-EOM
		Removing bundle: test-bundle1
		Deleting bundle files...
		Total deleted files: 2
		Removing bundle: test-bundle2
		Deleting bundle files...
		Total deleted files: 2
		Successfully removed 2 bundles
	EOM
	)
	assert_is_output --identical "$expected_output"

}

# ------------------------------------------
# Bad Cases (all bad bundles)
# ------------------------------------------

@test "bundle-remove output: removing one bundle that is not added" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle3"
	assert_status_is "$EBUNDLE_NOT_TRACKED"
	expected_output=$(cat <<-EOM
		Warning: Bundle "test-bundle3" is not installed, skipping it...
		Failed to remove 1 of 1 bundles
	EOM
	)
	assert_is_output --identical "$expected_output"

}

@test "bundle-remove output: removing one bundle that does not exist" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS fake-bundle"
	assert_status_is "$EBUNDLE_NOT_TRACKED"
	expected_output=$(cat <<-EOM
		Warning: Bundle "fake-bundle" is not installed, skipping it...
		Failed to remove 1 of 1 bundles
	EOM
	)
	assert_is_output --identical "$expected_output"

}

@test "bundle-remove output: removing multiple bundles, all invalid, one non existent, one already removed" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS fake-bundle test-bundle3"
	assert_status_is "$EBUNDLE_NOT_TRACKED"
	expected_output=$(cat <<-EOM
		Warning: Bundle "fake-bundle" is not installed, skipping it...
		Warning: Bundle "test-bundle3" is not installed, skipping it...
		Failed to remove 2 of 2 bundles
	EOM
	)
	assert_is_output --identical "$expected_output"

}

# ------------------------------------------
# Partial Cases (at least one good bundle)
# ------------------------------------------

@test "bundle-remove output: removing multiple bundles, one valid, one already removed, one non existent" {

	run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle3 fake-bundle test-bundle1"
	assert_status_is "$EBUNDLE_NOT_TRACKED"
	expected_output=$(cat <<-EOM
		Warning: Bundle "test-bundle3" is not installed, skipping it...
		Warning: Bundle "fake-bundle" is not installed, skipping it...
		Removing bundle: test-bundle1
		Deleting bundle files...
		Total deleted files: 2
		Failed to remove 2 of 3 bundles
	EOM
	)
	assert_is_output --identical "$expected_output"

}