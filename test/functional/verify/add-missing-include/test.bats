#!/usr/bin/env bats

load "../../swupdlib"

f1=491685caf9cf95df0f721254748df4717b4159513a3e0170ff5fa404dccc32e7
f2=826e0a73bdb6ca4863842b07ead83a55b478a950fefd2b1832b30046ce1c1550
f3=a96e0b959874854750e8e08372e62c4d1821c5e0106694365396d02c363ada50

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle1
  create_manifest_tar 10 test-bundle2
  create_fullfile_tar 10 $f1
  create_fullfile_tar 10 $f2
  create_fullfile_tar 10 $f3
}

teardown() {
  clean_tars 10
  clean_tars files
  revert_chown_root "$DIR/web-dir/10/files/$f1"
  revert_chown_root "$DIR/web-dir/10/files/$f2"
  revert_chown_root "$DIR/web-dir/10/files/$f3"
  sudo rm "$DIR/target-dir/os-core"
  sudo rm "$DIR/target-dir/test-bundle1"
  sudo rm "$DIR/target-dir/test-bundle2"
}

@test "verify add missing include" {
  run sudo sh -c "$SWUPD verify --fix $SWUPD_OPTS"

  check_lines "$output"
  [ -f "$DIR/target-dir/os-core" ]
  [ -f "$DIR/target-dir/test-bundle1" ]
  [ -f "$DIR/target-dir/test-bundle2" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
