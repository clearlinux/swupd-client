#!/usr/bin/env bats

load "../../swupdlib"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  create_manifest_tar 10 os-core
}

teardown() {
  clean_tars 10
  mkdir -p "$DIR/target-dir/testdir1/testdir2"
  touch "$DIR/target-dir/testdir1/testdir2/testfile"
}

@test "verify delete directory tree" {
  run sudo sh -c "$SWUPD verify --fix $SWUPD_OPTS"

  echo "$output"
  [ "${lines[5]}" = "Verifying version 10" ]
  [ "${lines[6]}" = "Attempting to download version string to memory" ]
  [ "${lines[7]}" = "Starting download of remaining update content. This may take a while..." ]
  [ "${lines[8]}" = "Finishing download of update content..." ]
  [ "${lines[9]}" = "Adding any missing files" ]
  [ "${lines[10]}" = "Fixing modified files" ]
  [ "${lines[11]}" = "Deleted $DIR/target-dir/testdir1/testdir2/testfile" ]
  [ "${lines[12]}" = "Deleted $DIR/target-dir/testdir1/testdir2" ]
  [ "${lines[13]}" = "Deleted $DIR/target-dir/testdir1" ]
  [ "${lines[15]}" = "  0 files were missing" ]
  [ "${lines[16]}" = "  3 files found which should be deleted" ]
  [ "${lines[17]}" = "    3 of 3 files were deleted" ]
  [ ! -d "$DIR/target-dir/testdir1" ]
  [ ! -d "$DIR/target-dir/testdir1/testdir2" ]
  [ ! -f "$DIR/target-dir/testdir1/testdir2/testfile" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
