#!/usr/bin/env bats

load "../../swupdlib"

dir_hash="6c27df6efcd6fc401ff1bc67c970b83eef115f6473db4fb9d57e5de317eba96e"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle
  create_fullfile_tar 10 $dir_hash
  chown_root "$DIR/web-dir/10/staged/$dir_hash"
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/pack-test-bundle-from-0.tar" --exclude=staged/$dir_hash/* staged/$dir_hash
}

teardown() {
  clean_tars 10
  revert_chown_root "$DIR/web-dir/10/staged/$dir_hash"
  sudo rmdir "$DIR/target-dir/usr/bin/"
}

@test "bundle-add add bundle containing a directory" {
  run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"
  echo "$status"
  [ "$status" -eq 0 ]
  check_lines "$output"
  [ -d "$DIR/target-dir/usr/bin" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
