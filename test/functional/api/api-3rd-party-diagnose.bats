#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME" 10 1

	# add a 3rd-party repo with some "findings" for diagnose
	create_third_party_repo -a "$TEST_NAME" 10 1 repo1
	create_bundle -L -t -n test-bundle1 -f /foo/file_1,/bar/file_2 -u repo1 "$TEST_NAME"
	create_version -p "$TEST_NAME" 20 10 1 repo1
	update_bundle -p "$TEST_NAME" test-bundle1 --update /foo/file_1 repo1
	update_bundle -p "$TEST_NAME" test-bundle1 --delete /bar/file_2 repo1
	update_bundle    "$TEST_NAME" test-bundle1 --add    /baz/file_3 repo1
	sudo touch "$TARGETDIR"/"$THIRD_PARTY_BUNDLES_DIR"/repo1/usr/untracked_file
	set_current_version "$TEST_NAME" 20 repo1

	# add another 3rd-party repo that has nothing to get fixed
	create_third_party_repo -a "$TEST_NAME" 10 1 repo2
	create_bundle -L -t -n test-bundle2 -f /baz/file_3 -u repo2 "$TEST_NAME"

}

@test "API055: 3rd-party diagnose" {

	run sudo sh -c "$SWUPD 3rd-party diagnose $SWUPD_OPTS --picky --quiet"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		[repo1]
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/baz
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/baz/file_3
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/foo/file_1
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/usr/lib/os-release
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/bar/file_2
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/usr/untracked_file
		[repo2]
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API056: 3rd-party diagnose --repo REPOSITORY" {

	run sudo sh -c "$SWUPD 3rd-party diagnose $SWUPD_OPTS --picky --repo repo1 --quiet"

	assert_status_is "$SWUPD_NO"
	expected_output=$(cat <<-EOM
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/baz
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/baz/file_3
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/foo/file_1
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/usr/lib/os-release
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/bar/file_2
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/usr/untracked_file
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=9
