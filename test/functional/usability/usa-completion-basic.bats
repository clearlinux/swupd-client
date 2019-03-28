#!/usr/bin/env bats

load "../testlib"

test_setup() {

	# do nothing
	return

}

test_teardown() {

	# do nothing
	return

}

@test "USA001: Verify the script for autocomplete exists" {

	assert_file_exists "$SWUPD_DIR"/swupd.bash

}

@test "USA002: Verify the autocomplete syntax" {

	bash "$SWUPD_DIR"/swupd.bash

}

@test "USA003: Autocomplete has autoupdate" {

	grep -q  '("autoupdate")' "$SWUPD_DIR"/swupd.bash

}

@test "USA004: Autocomplete has expected hashdump opts" {

	grep -q  'opts="--help --no-xattrs --path --debug --quiet "' "$SWUPD_DIR"/swupd.bash

}
