#!/usr/bin/env bats

load "../../swupdlib"

targetfile=6c27df6efcd6fc401ff1bc67c970b83eef115f6473db4fb9d57e5de317eba96e

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  create_manifest_tar 100 MoM
  sign_manifest_mom 100
  create_manifest_tar 100 os-core
  # create_pack_tar 10 100 os-core $targetfile
  chown_root "$DIR/web-dir/100/staged/$targetfile"
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/pack-os-core-from-10.tar" --exclude=staged/$targetfile/* staged/$targetfile
}

teardown() {
  clean_tars 10
  clean_tars 100
  revert_chown_root "$DIR/web-dir/100/staged/$targetfile"
}

@test "update only download packs" {
  run sudo sh -c "$SWUPD update $SWUPD_OPTS --download"

  check_lines "$output"
  [ ! -d "$DIR/target-dir/usr/bin" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
