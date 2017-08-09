#!/usr/bin/env bats

load "../../swupdlib"

f1=e324302d81e79a935137bd4ed6ab6c6792aa6d4356c9c227342eaa0db015c152
f2=826e0a73bdb6ca4863842b07ead83a55b478a950fefd2b1832b30046ce1c1550

setup() {
  clean_test_dir

  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle1
  create_manifest_tar 10 test-bundle2
  create_manifest_tar 10 test-bundle3

  create_fullfile_tar 10 $f1

  create_manifest_tar 20 MoM
  sign_manifest_mom 20
  create_manifest_tar 20 os-core

  create_manifest_tar 30 MoM
  sign_manifest_mom 30
  create_manifest_tar 30 os-core
  create_manifest_tar 30 test-bundle2
  create_manifest_tar 30 test-bundle3

  create_fullfile_tar 30 $f1
  create_fullfile_tar 30 $f2
}

teardown() {
  clean_tars 10
  clean_tars 20
  clean_tars 30
  clean_tars 10 files
  clean_tars 30 files
  revert_chown_root "$DIR/web-dir/10/files/$f1"
  revert_chown_root "$DIR/web-dir/30/files/$f1"
  revert_chown_root "$DIR/web-dir/30/files/$f2"
  sudo rm "$DIR/target-dir/os-core"
  sudo rm "$DIR/target-dir/tracked-file"
}

@test "update include a bundle from an older release" {
  run sudo sh -c "$SWUPD update $SWUPD_OPTS"

  [ "$status" -eq 0 ]
  check_lines "$output"

  # changed file
  [ -f "$DIR/target-dir/os-core" ]

  # new file
  [ -f "$DIR/target-dir/tracked-file" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
