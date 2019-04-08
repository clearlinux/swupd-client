#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

test_setup() {

	create_test_environment "$TEST_NAME"

	# pre-create two files in /usr/bin
	sudo mkdir -p "$TARGETDIR"/usr/bin
	write_to_protected_file "$TARGETDIR"/usr/bin/foo "old file"
	write_to_protected_file "$TARGETDIR"/usr/bin/bar "file that should remain"
	hash=$(sudo "$SWUPD" hashdump --quiet "$TARGETDIR"/usr/bin/bar)
	sudo cp "$TARGETDIR"/usr/bin/bar "$WEBDIR"/10/files/"$hash"

	# create a bundle that includes those two files, one file should match
	# the hash and the other one shouldn't
	create_bundle -n test-bundle -f /usr/bin/foo,/usr/bin/bar:"$WEBDIR"/10/files/"$hash" "$TEST_NAME"

}

@test "ADD050: Adding a bundle that overwrites an existing file" {

	# When swupd is installing a bundle and a file included in the bundle
	# already exists in the target system, it should replace the file if the
	# file is different

	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

	assert_status_is "$SWUPD_OK"
	assert_file_exists "$TARGETDIR"/usr/bin/foo
	assert_not_equal "old file" "$(cat "$TARGETDIR"/usr/bin/foo)"
	assert_file_exists "$TARGETDIR"/usr/bin/bar
	assert_equal "file that should remain" "$(cat "$TARGETDIR"/usr/bin/bar)"

}
