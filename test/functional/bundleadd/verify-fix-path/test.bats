#!/usr/bin/env bats

load "../../swupdlib"

t1_hash="6c27df6efcd6fc401ff1bc67c970b83eef115f6473db4fb9d57e5de317eba96e"
t2_hash="e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle
  create_fullfile_tar 10 $t1_hash
  chown_root "$DIR/web-dir/10/staged/$t2_hash"
  sudo tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/pack-test-bundle-from-0.tar" staged/$t2_hash
  chown_root "$DIR/target-dir/usr"
  sudo chmod 755 "$DIR/target-dir/usr"
}

teardown() {
  clean_tars 10
  clean_tars 10 files
  revert_chown_root "$DIR/web-dir/10/files/$t1_hash"
  revert_chown_root "$DIR/web-dir/10/staged/$t2_hash"
  revert_chown_root "$DIR/target-dir/usr"
  sudo rm "$DIR/target-dir/usr/bin/foo"
  sudo rmdir "$DIR/target-dir/usr/bin/"
}

@test "bundle-add verify_fix_path support" {
  run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

  echo "$output"
  [ "${lines[2]}" = "Attempting to download version string to memory" ]
  ignore_sigverify_error 3
  [ "${lines[3]}" = "Downloading packs..." ]
  [ "${lines[4]}" = "Downloading test-bundle pack for version 10" ]
  [ "${lines[5]}" = "Extracting pack." ]
  [ "${lines[6]}" = "Installing bundle(s) files..." ]
  [ "${lines[8]}" = "Path /usr/bin is missing on the file system" ]
  [ "${lines[12]}" = "Bundle(s) installation done." ]
  [ -d "$DIR/target-dir/usr/bin" ]
  [ -f "$DIR/target-dir/usr/bin/foo" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
