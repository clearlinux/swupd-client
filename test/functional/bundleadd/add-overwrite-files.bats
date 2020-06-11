#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"

	# pre-create two files in /usr/bin
	sudo mkdir -p "$TARGET_DIR"/usr/bin
	write_to_protected_file "$TARGET_DIR"/usr/bin/foo "should be replaced"
	write_to_protected_file "$TARGET_DIR"/usr/bin/bar "should NOT be replaced"

	# create a bundle with one of the existing files and install the bundle
	hash=$(sudo "$SWUPD" hashdump --quiet "$TARGET_DIR"/usr/bin/bar)
	sudo cp "$TARGET_DIR"/usr/bin/bar "$WEB_DIR"/10/files/"$hash"
	create_bundle -L -n test-bundle1 -f /usr/bin/bar:"$WEB_DIR"/10/files/"$hash" "$TEST_NAME"
	create_tar "$WEB_DIR"/10/files/"$hash"

	# create another bundle that includes two files:
	# - /usr/bin/foo: not tracked but already existing in the target system (should be replaced)
	# - /usr/bin/bar: already tracked by another installed bundle (should not be replaced)
	create_bundle -n test-bundle2 -f /usr/bin/foo,/usr/bin/bar:"$WEB_DIR"/10/files/"$hash" "$TEST_NAME"

	create_bundle -n test-bundle3 -f /file1,/file2,/file3 "$TEST_NAME"
	add_dependency_to_manifest "$WEB_DIR"/10/Manifest.test-bundle2 test-bundle3

}

@test "ADD050: Adding a bundle that overwrites an existing file" {

	# When swupd is installing a bundle and a file included in the bundle
	# already exists in the target system, it should replace the file if the
	# file is different

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle2"

	assert_status_is "$SWUPD_OK"
	assert_file_exists "$TARGET_DIR"/usr/bin/foo
	assert_not_equal "should be replaced" "$(cat "$TARGET_DIR"/usr/bin/foo)"
	assert_file_exists "$TARGET_DIR"/usr/bin/bar
	assert_equal "should NOT be replaced" "$(cat "$TARGET_DIR"/usr/bin/bar)"
	assert_file_exists "$TARGET_DIR"/file1

}
#WEIGHT=4
