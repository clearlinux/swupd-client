#!/usr/bin/env bats

load "../../swupdlib"

f_hash="e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle
  chown_root "$DIR/web-dir/10/staged/$f_hash"
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/pack-test-bundle-from-0.tar" staged/$f_hash
}

teardown() {
  clean_tars 10
  revert_chown_root "$DIR/web-dir/10/staged/$f_hash"
  sudo rm "$DIR/target-dir/usr/lib/kernel/testfile"
}

@test "bundle-add add bundle containing boot file with boot update override" {
  run sudo sh -c "$SWUPD bundle-add -b $SWUPD_OPTS test-bundle"

  [ "$status" -eq 0 ]
  check_lines "$output"
  echo $output
  [ -f "$DIR/target-dir/usr/lib/kernel/testfile" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
