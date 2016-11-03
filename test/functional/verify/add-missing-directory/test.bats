#!/usr/bin/env bats

load "../../swupdlib"

targetfile=6c27df6efcd6fc401ff1bc67c970b83eef115f6473db4fb9d57e5de317eba96e

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  create_manifest_tar 10 os-core
  create_fullfile_tar 10 $targetfile
}

teardown() {
  clean_tars 10
  clean_tars 10 files
  revert_chown_root "$DIR/web-dir/10/files/$targetfile"
  sudo rmdir "$DIR/target-dir/usr/bin/"
}

@test "verify add missing directory" {
  run sudo sh -c "$SWUPD verify --fix $SWUPD_OPTS"

  check_lines "$output"
  [ -d "$DIR/target-dir/usr/bin" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
