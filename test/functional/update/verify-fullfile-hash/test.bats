#!/usr/bin/env bats

load "../../swupdlib"

f1=520f83440d3dddc25ad09ca858b9c669245f82d3181a45cdfe793aac9dd1fb15

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle
  create_manifest_tar 100 MoM
  sign_manifest_mom 100
  create_manifest_tar 100 test-bundle
  create_fullfile_tar 100 $f1
}

teardown() {
  clean_tars 10
  clean_tars 100
  clean_tars 100 files
  revert_chown_root "$DIR/web-dir/100/files/$f1"
}

@test "update fullfile hashes verified" {
  run sudo sh -c "$SWUPD update $SWUPD_OPTS"

  check_lines "$output"

  [ ! -f "$DIR/target-dir/foo" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
