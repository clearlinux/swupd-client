#!/usr/bin/env bats

load "../../swupdlib"

f1=6c27df6efcd6fc401ff1bc67c970b83eef115f6473db4fb9d57e5de317eba96e

setup() {
  clean_test_dir

  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle1
  create_manifest_tar 100 MoM
  sign_manifest_mom 100
  create_manifest_tar 100 os-core
  create_manifest_tar 100 test-bundle1

  create_fullfile_tar 100 $f1
}

teardown() {
  clean_tars 10
  clean_tars 100
  clean_tars 100 files
  revert_chown_root "$DIR/web-dir/100/files/$f1"
  sudo rmdir "$DIR/target-dir/os-core"
  sudo rmdir "$DIR/target-dir/usr/bin"
}

@test "update always add os-core to tracked bundles" {
  run sudo sh -c "$SWUPD update $SWUPD_OPTS"

  [ "$status" -eq 0 ]
  check_lines "$output"

  # changed files (hashes didn't change, only the versions)
  [ -d "$DIR/target-dir/os-core" ]
  [ -d "$DIR/target-dir/usr/bin" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
