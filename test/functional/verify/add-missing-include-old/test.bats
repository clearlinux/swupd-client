#!/usr/bin/env bats

load "../../swupdlib"

f1=491685caf9cf95df0f721254748df4717b4159513a3e0170ff5fa404dccc32e7
f2=826e0a73bdb6ca4863842b07ead83a55b478a950fefd2b1832b30046ce1c1550
f3=a96e0b959874854750e8e08372e62c4d1821c5e0106694365396d02c363ada50

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle1
  create_manifest_tar 10 test-bundle2
  create_fullfile_tar 10 $f1

  create_manifest_tar 100 MoM
  create_manifest_tar 100 os-core
  create_manifest_tar 100 test-bundle1
  create_fullfile_tar 100 $f2
  create_fullfile_tar 100 $f3
}

teardown() {
  clean_tars 10
  clean_tars 10 files
  clean_tars 100
  clean_tars files
  revert_chown_root "$DIR/web-dir/10/files/$f1"
  revert_chown_root "$DIR/web-dir/100/files/$f2"
  revert_chown_root "$DIR/web-dir/100/files/$f3"
  sudo rm "$DIR/target-dir/os-core"
  sudo rm "$DIR/target-dir/test-bundle1"
  sudo rm "$DIR/target-dir/test-bundle2"
}

@test "verify add missing old include" {
  run sudo sh -c "$SWUPD verify --fix $SWUPD_OPTS"

  echo "$output"
  [ "${lines[5]}" = "Verifying version 100" ]
  [ "${lines[6]}" = "Attempting to download version string to memory" ]
  ignore_sigverify_error 7
  [ "${lines[7]}" = "Starting download of remaining update content. This may take a while..." ]
  [ "${lines[8]}" = "Finishing download of update content..." ]
  [ "${lines[9]}" = "Adding any missing files" ]
  [ "${lines[16]}" = "Fixing modified files" ]
  [ "${lines[17]}" = "Inspected 3 files" ]
  [ "${lines[18]}" = "  3 files were missing" ]
  [ "${lines[19]}" = "    3 of 3 missing files were replaced" ]
  [ "${lines[20]}" = "    0 of 3 missing files were not replaced" ]
  [ "${lines[21]}" = "  0 files found which should be deleted" ]
  [ "${lines[23]}" = "Fix successful" ]
  [ -f "$DIR/target-dir/os-core" ]
  [ -f "$DIR/target-dir/test-bundle1" ]
  [ -f "$DIR/target-dir/test-bundle2" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
