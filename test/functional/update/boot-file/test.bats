#!/usr/bin/env bats

load "../../swupdlib"

targetfile=e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  create_manifest_tar 100 MoM
  sign_manifest_mom 100
  create_manifest_tar 100 os-core
  create_fullfile_tar 100 $targetfile
}

teardown() {
  clean_tars 10
  clean_tars 100
  clean_tars 100 files
  revert_chown_root "$DIR/web-dir/100/files/$targetfile"
  sudo rm "$DIR/target-dir/usr/lib/kernel/testfile"
}

@test "update add boot file" {
  run sudo sh -c "$SWUPD update $SWUPD_OPTS"

  check_lines "$output"
  [ -f "$DIR/target-dir/usr/lib/kernel/testfile" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
