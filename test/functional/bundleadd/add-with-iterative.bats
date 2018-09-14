#!/usr/bin/env bats

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle-1 -f /foo,/baz/test-file1 "$TEST_NAME"
	create_bundle -n test-bundle-2 -f /bar,/baz/test-file2 "$TEST_NAME"
	create_version "$TEST_NAME" 20 10
	update_bundle -i "$TEST_NAME" test-bundle-1 --update /baz/test-file1
	create_version "$TEST_NAME" 30 20
	update_bundle -i "$TEST_NAME" test-bundle-2 --delete /baz/test-file2

}

@test "bundle-add add bundle works fine when there are iterative manifests in other versions" {

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle-1 test-bundle-2"

	assert_status_is 0

	# verify target files
	assert_file_exists "$TARGETDIR"/foo
	assert_file_exists "$TARGETDIR"/bar
	assert_file_exists "$TARGETDIR"/baz/test-file1
	assert_file_exists "$TARGETDIR"/baz/test-file2
	# verify downloaded manifests
	assert_file_exists "$STATEDIR"/10/Manifest.test-bundle-1
	assert_file_exists "$STATEDIR"/10/Manifest.test-bundle-2

	expected_output=$(cat <<-EOM
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Installing bundle(s) files...
		Calling post-update helper scripts.
		Successfully installed 2 bundles
	EOM
	)
	assert_is_output "$expected_output"

}

@test "bundle-add add bundle does not use iterative manifests" {

	# pre-requisiste
	set_current_version "$TEST_NAME" 30

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle-1 test-bundle-2"

	assert_status_is 0

	# verify target files
	assert_file_exists "$TARGETDIR"/foo
	assert_file_exists "$TARGETDIR"/bar
	assert_file_exists "$TARGETDIR"/baz/test-file1
	assert_file_not_exists "$TARGETDIR"/baz/test-file2
	# verify downloaded manifests
	assert_file_exists "$STATEDIR"/20/Manifest.test-bundle-1
	assert_file_exists "$STATEDIR"/30/Manifest.test-bundle-2
	# verify not necessary manifests are not downloaded
	assert_file_not_exists "$STATEDIR"/20/Manifest.test-bundle-1.I.10
	assert_file_not_exists "$STATEDIR"/30/Manifest.test-bundle-2.I.10

	expected_output=$(cat <<-EOM
		Starting download of remaining update content. This may take a while...
		Finishing download of update content...
		Installing bundle(s) files...
		Calling post-update helper scripts.
		Successfully installed 2 bundles
	EOM
	)
	assert_is_output "$expected_output"

}
