#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

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

@test "API060: 3rd-party repair" {

	run sudo sh -c "$SWUPD 3rd-party repair $SWUPD_OPTS --picky --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		[repo1]
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/baz -> fixed
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/baz/file_3 -> fixed
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/foo/file_1 -> fixed
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/usr/lib/os-release -> fixed
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/bar/file_2 -> deleted
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/usr/untracked_file -> deleted
		[repo2]
	EOM
	)
	assert_is_output "$expected_output"

}

@test "API061: 3rd-party repair --repo REPOSITORY" {

	run sudo sh -c "$SWUPD 3rd-party repair $SWUPD_OPTS --picky --repo repo1 --quiet"

	assert_status_is "$SWUPD_OK"
	expected_output=$(cat <<-EOM
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/baz -> fixed
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/baz/file_3 -> fixed
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/foo/file_1 -> fixed
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/usr/lib/os-release -> fixed
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/bar/file_2 -> deleted
		$PATH_PREFIX/opt/3rd-party/bundles/repo1/usr/untracked_file -> deleted
	EOM
	)
	assert_is_output "$expected_output"

}
#WEIGHT=17
