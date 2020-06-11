#!/usr/bin/env bats

load "../testlib"

@test "USA001: Verify the script for autocomplete exists" {

	assert_file_exists "$ABS_SWUPD_DIR"/swupd.bash

}

@test "USA002: Verify the autocomplete syntax" {

	bash --posix "$ABS_SWUPD_DIR"/swupd.bash

}

@test "USA003: Autocomplete has autoupdate" {

	grep -q  '("bundle-add")' "$ABS_SWUPD_DIR"/swupd.bash

}

@test "USA004: Autocomplete has expected hashdump" {

	grep -q  '("hashdump")' "$ABS_SWUPD_DIR"/swupd.bash

}
#WEIGHT=1
