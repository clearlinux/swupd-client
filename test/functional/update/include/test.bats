#!/usr/bin/env bats

load "../../swupdlib"

f1=6c27df6efcd6fc401ff1bc67c970b83eef115f6473db4fb9d57e5de317eba96e
f2=5f79d142c1859a3d82e9391a9a23a5f697e9346714412b67c78729c5a7b6ae1f
f3=a325dc221e0e03802b5a81abdf7318074230ca1f980985032048b5b6ce193850
f4=d1f70150c56e960ca18bd636131fae747487ac93836429ada2a46f85b88af28f
f5=4185aaf172a7f8b0cffb79fa379dc2227edb7097f24a61f7ce5b3881e38a66a2
f6=e2c7d7bb180e8b44ce0fb4e58f3b2f3852d7fbbb0fafbe04e41159d006501a88
f7=01fe90f246616cc07c216fa4fc1444881a5f2804650989996a44d223e0bfb105
f8=9a94d945b3841b70df7117fd02cffa36e4f1e5b430703b8fd26c086bf1059e72

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle1
  create_manifest_tar 10 test-bundle2
  create_manifest_tar 100 MoM
  sign_manifest_mom 100
  create_manifest_tar 100 os-core
  create_manifest_tar 100 test-bundle1
  create_manifest_tar 100 test-bundle2
  create_manifest_tar 100 test-bundle3
  create_manifest_tar 100 test-bundle4
  create_manifest_tar 100 test-bundle5
  create_manifest_tar 100 test-bundle6
  create_manifest_tar 100 test-bundle7
  create_fullfile_tar 100 $f1
  create_fullfile_tar 100 $f2
  create_fullfile_tar 100 $f3
  create_fullfile_tar 100 $f4
  create_fullfile_tar 100 $f5
  create_fullfile_tar 100 $f6
  create_fullfile_tar 100 $f7
  create_fullfile_tar 100 $f8
}

teardown() {
  clean_tars 10
  clean_tars 100
  clean_tars 100 files
  revert_chown_root "$DIR/web-dir/100/files/$f1"
  revert_chown_root "$DIR/web-dir/100/files/$f2"
  revert_chown_root "$DIR/web-dir/100/files/$f3"
  revert_chown_root "$DIR/web-dir/100/files/$f4"
  revert_chown_root "$DIR/web-dir/100/files/$f5"
  revert_chown_root "$DIR/web-dir/100/files/$f6"
  revert_chown_root "$DIR/web-dir/100/files/$f7"
  revert_chown_root "$DIR/web-dir/100/files/$f8"
  sudo rmdir "$DIR/target-dir/usr/bin"
  sudo rm "$DIR/target-dir/usr/core"
  sudo rm "$DIR/target-dir/usr/foo2"
  sudo rm "$DIR/target-dir/usr/foo3"
  sudo rm "$DIR/target-dir/usr/foo4"
  sudo rm "$DIR/target-dir/usr/foo5"
  sudo rm "$DIR/target-dir/usr/foo6"
  sudo rm "$DIR/target-dir/usr/foo7"
}

@test "update with includes" {
  run sudo sh -c "$SWUPD update $SWUPD_OPTS"

  check_lines "$output"

  # changed files
  [ -d "$DIR/target-dir/usr/bin" ]
  [ -f "$DIR/target-dir/usr/core" ]

  # new files
  [ -f "$DIR/target-dir/usr/foo2" ]
  [ -f "$DIR/target-dir/usr/foo3" ]
  [ -f "$DIR/target-dir/usr/foo4" ]
  [ -f "$DIR/target-dir/usr/foo5" ]
  [ -f "$DIR/target-dir/usr/foo6" ]
  [ -f "$DIR/target-dir/usr/foo7" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
