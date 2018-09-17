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

@test "autocomplete exists" {

	assert_file_exists "$SWUPD_DIR"/swupd.bash

}

@test "autocomplete syntax" {

	bash "$SWUPD_DIR"/swupd.bash

}

@test "autocomplete has autoupdate" {

	grep -q  '("autoupdate")' "$SWUPD_DIR"/swupd.bash

}

@test "autocomplete has expected hashdump opts" {

	grep -q  'opts="--help --no-xattrs --path "' "$SWUPD_DIR"/swupd.bash

}
