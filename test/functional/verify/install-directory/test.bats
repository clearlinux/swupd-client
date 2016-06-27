#!/usr/bin/env bats

load "../../swupdlib"

targetfile=24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
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

  echo "$output"
  [ "${lines[5]}" = "Verifying version 10" ]
  [ "${lines[6]}" = "Attempting to download version string to memory" ]
  ignore_sigverify_error 7
  [ "${lines[7]}" = "Downloading os-core pack for version 10" ]
  [ "${lines[8]}" = "Extracting pack." ]
  [ "${lines[9]}" = "Adding any missing files" ]
  [ "${lines[10]}" = "Inspected 1 files" ]
  [ "${lines[11]}" = "  1 files were missing" ]
  [ "${lines[12]}" = "    1 of 1 missing files were replaced" ]
  [ "${lines[13]}" = "    0 of 1 missing files were not replaced" ]
  [ "${lines[15]}" = "Fix successful" ]
  [ -d "$DIR/target-dir/usr/bin" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
