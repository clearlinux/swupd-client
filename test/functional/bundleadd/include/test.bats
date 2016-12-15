#!/usr/bin/env bats

load "../../swupdlib"

t1_hash="24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af"
t2_hash="e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle
  chown_root "$DIR/web-dir/10/staged/$t1_hash"
  chown_root "$DIR/web-dir/10/staged/$t2_hash"
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/pack-os-core-from-0.tar" --exclude=staged/$t1_hash/* staged/$t1_hash
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/pack-test-bundle-from-0.tar" staged/$t2_hash
}

teardown() {
  clean_tars 10
  revert_chown_root "$DIR/web-dir/10/staged/$t1_hash"
  revert_chown_root "$DIR/web-dir/10/staged/$t2_hash"
  sudo rmdir "$DIR/target-dir/usr/bin/"
  sudo rm "$DIR/target-dir/usr/foo"
}

@test "bundle-add verify include support" {
  run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

  check_lines "$output"
  ls "$DIR/target-dir/usr/bin"
  ls "$DIR/target-dir/usr/foo"
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
