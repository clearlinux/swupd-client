#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /foo/test-file1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /bar/test-file2 "$TEST_NAME"
	create_bundle -n test-bundle3 -f /bar/test-file3 "$TEST_NAME"
	# add test-bundle2 as a dependency of test-bundle1
	add_dependency_to_manifest "$WEBDIR"/10/Manifest.test-bundle1 test-bundle2

}

@test "ADD013: Adding a bundle that includes another bundle" {

	# Add bundle 3 to prime the bundles statedir contents so the tracked
	# bundle detection can work in a realistic way
	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle3"
	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1"

	assert_status_is "$SWUPD_OK"
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/bar/test-file2
	assert_file_exists "$STATEDIR"/bundles/test-bundle1
	assert_file_exists "$STATEDIR"/bundles/test-bundle3
	assert_file_not_exists "$STATEDIR"/bundles/test-bundle2
	expected_output=$(cat <<-EOM
		Loading required manifests...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Calling post-update helper scripts
		Successfully installed 1 bundle
		1 bundle was installed as dependency
	EOM
	)
	assert_is_output "$expected_output"

}

@test "ADD048: Adding a bundle that includes another bundle (no tracking state)" {

	# simulate as if test-bundle3 is already installed
	sudo touch "$TARGETDIR"/usr/share/clear/bundles/test-bundle3
	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1"

	assert_status_is "$SWUPD_OK"
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/bar/test-file2
	assert_file_exists "$STATEDIR"/bundles/test-bundle1
	assert_file_exists "$STATEDIR"/bundles/test-bundle3
	# When no tracking directly previously existed, all currently installed
	# bundles are added to the tracked state.
	assert_file_not_exists "$STATEDIR"/bundles/test-bundle2
	expected_output=$(cat <<-EOM
		Loading required manifests...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Calling post-update helper scripts
		Successfully installed 1 bundle
		1 bundle was installed as dependency
	EOM
	)
	assert_is_output "$expected_output"

}

@test "ADD053: Adding two bundles, one of them includes the other one as dependency" {

	# when adding two bundles, and one is a dependency of the other one
	# swupd should still count/identify the bundles correctly, it should only
	# count the bundle as dependency if it was not directly added by the user
	# regardless of the order of the bundles requested

	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$STATEDIR"/bundles/test-bundle1
	assert_file_not_exists "$TARGETDIR"/foo/test-file1
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_not_exists "$STATEDIR"/bundles/test-bundle2
	assert_file_not_exists "$TARGETDIR"/bar/test-file2

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle2 test-bundle1"

	assert_status_is "$SWUPD_OK"
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_exists "$STATEDIR"/bundles/test-bundle1
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$STATEDIR"/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/bar/test-file2

	expected_output=$(cat <<-EOM
		Loading required manifests...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Calling post-update helper scripts
		Successfully installed 2 bundles
	EOM
	)
	assert_is_output "$expected_output"

	remove_bundle -L "$TEST_NAME"/web-dir/10/Manifest.test-bundle1
	remove_bundle -L "$TEST_NAME"/web-dir/10/Manifest.test-bundle2
	clean_state_dir "$TEST_NAME"

	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$STATEDIR"/bundles/test-bundle1
	assert_file_not_exists "$TARGETDIR"/foo/test-file1
	assert_file_not_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_not_exists "$STATEDIR"/bundles/test-bundle2
	assert_file_not_exists "$TARGETDIR"/bar/test-file2

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1 test-bundle2"

	assert_status_is "$SWUPD_OK"
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle1
	assert_file_exists "$STATEDIR"/bundles/test-bundle1
	assert_file_exists "$TARGETDIR"/foo/test-file1
	assert_file_exists "$TARGETDIR"/usr/share/clear/bundles/test-bundle2
	assert_file_exists "$STATEDIR"/bundles/test-bundle2
	assert_file_exists "$TARGETDIR"/bar/test-file2

	expected_output=$(cat <<-EOM
		Loading required manifests...
		No packs need to be downloaded
		Validate downloaded files
		Starting download of remaining update content. This may take a while...
		Installing files...
		Calling post-update helper scripts
		Successfully installed 2 bundles
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=9
