#!/usr/bin/env bats

load "../../swupdlib"

# expected hash listed in manifest, but not acutal hash of file
# also file name in staged directory
f_hash="e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle
  create_fullfile_tar 10 $f_hash
  chown_root "$DIR/web-dir/10/staged/$f_hash"
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/pack-test-bundle-from-0.tar" staged/$f_hash
}

teardown() {
  clean_tars 10
  revert_chown_root "$DIR/web-dir/10/staged/$f_hash"
}

@test "bundle-add add bundle containing file with different hash from what is listed in manifest" {
  run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

  # downloaded fullfile had a bad hash - immediately fatal with a 1 return code
  [ "$status" -eq 1 ]
  check_lines "$output"
  # the bad hash file should not exist on the system
  [ ! -f "$DIR/target-dir/testfile" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
