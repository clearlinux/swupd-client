#!/usr/bin/env bats

load "../testlib"

@test "USA001: Verify the script for autocomplete exists" {

	assert_file_exists "$SWUPD_DIR"/swupd.bash

}

@test "USA002: Verify the autocomplete syntax" {

	bash --posix "$SWUPD_DIR"/swupd.bash

}

@test "USA003: Autocomplete has autoupdate" {

	grep -q  '("bundle-add")' "$SWUPD_DIR"/swupd.bash

}

@test "USA004: Autocomplete has expected hashdump" {

	grep -q  '("hashdump")' "$SWUPD_DIR"/swupd.bash

}
#WEIGHT=1
