#!/usr/bin/env bats

load "../../swupdlib"

targetfile=24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af

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

  echo "$output"
  [ "${lines[5]}" = "Verifying version 10" ]
  [ "${lines[6]}" = "Attempting to download version string to memory" ]
  ignore_sigverify_error 7
  [ "${lines[7]}" = "Starting download of remaining update content. This may take a while..." ]
  [ "${lines[8]}" = "Finishing download of update content..." ]
  [ "${lines[9]}" = "Adding any missing files" ]
  [ "${lines[12]}" = "Fixing modified files" ]
  [ "${lines[13]}" = "Inspected 1 files" ]
  [ "${lines[14]}" = "  1 files were missing" ]
  [ "${lines[15]}" = "    1 of 1 missing files were replaced" ]
  [ "${lines[16]}" = "    0 of 1 missing files were not replaced" ]
  [ "${lines[17]}" = "  0 files found which should be deleted" ]
  [ "${lines[19]}" = "Fix successful" ]
  [ -d "$DIR/target-dir/usr/bin" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
