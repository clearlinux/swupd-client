#!/usr/bin/env bats

load "../../swupdlib"

targetfile=e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  chown_root "$DIR/target-dir/usr/lib/testfile"
  create_fullfile_tar 10 $targetfile
}

teardown() {
  clean_tars 10
  clean_tars 10 files
  revert_chown_root "$DIR/web-dir/10/files/$targetfile"
  sudo rm "$DIR/target-dir/usr/lib/testfile"
  cp "$DIR/web-dir/10/files/$targetfile" "$DIR/target-dir/usr/lib/testfile"
}

@test "verify override post-update scripts" {
  run sudo sh -c "$SWUPD verify --fix --no-scripts $SWUPD_OPTS"

  # check for the warning
  check_lines "$output"
  # this should exist at the end, even if the scripts are not run
  [ -f "$DIR/target-dir/usr/lib/testfile" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
