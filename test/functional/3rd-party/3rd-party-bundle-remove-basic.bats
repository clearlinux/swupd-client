#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -L -n upstream-bundle -f /upstream_file "$TEST_NAME"

	# create a couple 3rd-party repos within the test environment and add
	# some bundles to them
	create_third_party_repo -a "$TEST_NAME" 10 1 test-repo1
	create_bundle -L -t -n test-bundle1 -f /foo/file_1 -u test-repo1 "$TEST_NAME"

	create_third_party_repo -a "$TEST_NAME" 10 1 test-repo2
	create_bundle -L -t -n test-bundle2 -f /foo/file_2 -u test-repo2 "$TEST_NAME"
	create_bundle -L -t -n test-bundle3 -f /bar/file_3 -u test-repo2 "$TEST_NAME"

}

@test "TPR024: Removing one bundle from a third party repo" {

	run sudo sh -c "$SWUPD 3rd-party bundle-remove $SWUPD_OPTS test-bundle2"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle2 in the 3rd-party repositories...
		Bundle test-bundle2 found in 3rd-party repository test-repo2
		The following bundles are being removed:
		 - test-bundle2
		Deleting bundle files...
		Total deleted files: 3
		Removing 3rd-party bundle binaries...
		Successfully removed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo2/foo/file_2
	assert_file_not_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo2/usr/share/clear/bundles/test-bundle2
	assert_file_not_exists "$TPSTATEDIR"/bundles/test-bundle2

}

@test "TPR025: Try removing one invalid bundle from a third party repo" {

	run sudo sh -c "$SWUPD 3rd-party bundle-remove $SWUPD_OPTS upstream-bundle"

	assert_status_is "$SWUPD_INVALID_BUNDLE"
	expected_output=$(cat <<-EOM
		Searching for bundle upstream-bundle in the 3rd-party repositories...
		Error: bundle upstream-bundle was not found in any 3rd-party repository
	EOM
	)
	assert_is_output "$expected_output"

	# the same should happen with an invalid bundle
	run sudo sh -c "$SWUPD 3rd-party bundle-add $SWUPD_OPTS fake-bundle"

	assert_status_is "$SWUPD_INVALID_BUNDLE"
	expected_output=$(cat <<-EOM
		Searching for bundle fake-bundle in the 3rd-party repositories...
		Error: bundle fake-bundle was not found in any 3rd-party repository
	EOM
	)
	assert_is_output "$expected_output"

}

@test "TPR026: Try removing one valid bundle from a third party repo and one invalid" {

	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo1/foo/file_1
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo1/usr/share/clear/bundles/test-bundle1
	assert_file_exists "$STATEDIR"/3rd-party/test-repo1/bundles/test-bundle1

	run sudo sh -c "$SWUPD 3rd-party bundle-remove $SWUPD_OPTS test-bundle1 upstream-bundle"

	assert_status_is "$SWUPD_INVALID_BUNDLE"
	expected_output=$(cat <<-EOM
		Searching for bundle test-bundle1 in the 3rd-party repositories...
		Bundle test-bundle1 found in 3rd-party repository test-repo1
		The following bundles are being removed:
		 - test-bundle1
		Deleting bundle files...
		Total deleted files: 3
		Removing 3rd-party bundle binaries...
		Successfully removed 1 bundle
		Searching for bundle upstream-bundle in the 3rd-party repositories...
		Error: bundle upstream-bundle was not found in any 3rd-party repository
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo1/foo/file_1
	assert_file_not_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo1/usr/share/clear/bundles/test-bundle1
	assert_file_not_exists "$STATEDIR"/3rd-party/test-repo1/bundles/test-bundle1

	# run the same scenario with the arguments switched
	install_bundle "$TEST_NAME"/3rd-party/test-repo1/10/Manifest.test-bundle1 test-repo1
	clean_state_dir "$TEST_NAME" test-repo1
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo1/foo/file_1
	assert_file_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo1/usr/share/clear/bundles/test-bundle1

	run sudo sh -c "$SWUPD 3rd-party bundle-remove $SWUPD_OPTS upstream-bundle test-bundle1"

	assert_status_is "$SWUPD_INVALID_BUNDLE"
	expected_output=$(cat <<-EOM
		Searching for bundle upstream-bundle in the 3rd-party repositories...
		Error: bundle upstream-bundle was not found in any 3rd-party repository
		Searching for bundle test-bundle1 in the 3rd-party repositories...
		Bundle test-bundle1 found in 3rd-party repository test-repo1
		The following bundles are being removed:
		 - test-bundle1
		Deleting bundle files...
		Total deleted files: 3
		Removing 3rd-party bundle binaries...
		Successfully removed 1 bundle
	EOM
	)
	assert_is_output "$expected_output"
	assert_file_not_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo1/foo/file_1
	assert_file_not_exists "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/test-repo1/usr/share/clear/bundles/test-bundle1

}
#WEIGHT=24
