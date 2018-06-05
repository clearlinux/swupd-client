#!/usr/bin/env bats

load "../../swupdlib"

targetfile=6c27df6efcd6fc401ff1bc67c970b83eef115f6473db4fb9d57e5de317eba96e

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  chown_root "$DIR/web-dir/10/staged/$targetfile"
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/pack-os-core-from-0.tar" --exclude=staged/$targetfile/* staged/$targetfile
}

teardown() {
  clean_tars 10
  revert_chown_root "$DIR/web-dir/10/staged/$targetfile"
  sudo rmdir "$DIR/target-dir/usr/bin/"
}

@test "verify install directory" {
  run sudo sh -c "$SWUPD verify $SWUPD_OPTS --install -m 10"

  check_lines "$output"
  [ -d "$DIR/target-dir/usr/bin" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
