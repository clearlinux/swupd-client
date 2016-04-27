#!/usr/bin/env bats

load "../../swupdlib"

setup() {
  clean_test_dir
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.MoM.tar" Manifest.MoM Manifest.MoM.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.os-core.tar" Manifest.os-core Manifest.os-core.signed
}

teardown() {
  pushd "$DIR/web-dir/10"
  rm *.tar
  popd
  mkdir -p "$DIR/target-dir/testdir1/testdir2"
  touch "$DIR/target-dir/testdir1/testdir2/testfile"
}

@test "verify delete directory tree" {
  run sudo sh -c "$SWUPD verify --fix $SWUPD_OPTS"

  echo "$output"
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
