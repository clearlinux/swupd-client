#!/usr/bin/env bats

load "../../swupdlib"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  create_manifest_tar 10 os-core
  rm "$DIR/target-dir/testdir/.gitignore"
}

teardown() {
  clean_tars 10
  mkdir -p "$DIR/target-dir/testdir"
  touch "$DIR/target-dir/testdir/.gitignore"
}

@test "verify delete already removed directory" {
  run sudo sh -c "$SWUPD verify --fix $SWUPD_OPTS"

  echo "$output"
  [ "${lines[5]}" = "Verifying version 10" ]
  [ "${lines[6]}" = "Attempting to download version string to memory" ]
  [ "${lines[7]}" = "Starting download of remaining update content. This may take a while..." ]
  [ "${lines[8]}" = "Finishing download of update content..." ]
  [ "${lines[9]}" = "Adding any missing files" ]
  [ "${lines[10]}" = "Fixing modified files" ]
  [ "${lines[12]}" = "Inspected 0 files" ]
  [ "${lines[13]}" = "  0 files were missing" ]
  [ "${lines[14]}" = "  1 files found which should be deleted" ]
  [ "${lines[15]}" = "    1 of 1 files were deleted" ]
  [ "${lines[16]}" = "    0 of 1 files were not deleted" ]
  [ "${lines[18]}" = "Fix successful" ]
  [ ! -d "$DIR/target-dir/testdir" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
