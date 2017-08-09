#!/usr/bin/env bats

load "../../swupdlib"

dir_hash="24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle
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
  [ "$status" -eq 0 ]
  check_lines "$output"
  [ -d "$DIR/target-dir/usr/bin" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
