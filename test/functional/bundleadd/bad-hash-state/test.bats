#!/usr/bin/env bats

load "../../swupdlib"

f_hash="ffbe308091534f3b47b876b241166c4c6d0665204977274093f74851380d49b5"

setup() {
  # set up state directory with bad hash file and pack hint
  clean_test_dir
  sudo mkdir -p $DIR/state/staged
  sudo rm -f "$DIR/state/staged/$f_hash"
  sudo sh -c "echo \"test file MODIFIED\" > $DIR/state/staged/$f_hash"
  sudo touch $DIR/state/pack-test-bundle-from-0-to-10.tar
  chown_root -R $DIR/state
  sudo chmod -R 0700 $DIR/state

  # set up web dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle
  create_fullfile_tar 10 $f_hash
  tar -C $DIR/web-dir/10 -cf $DIR/web-dir/10/pack-test-bundle-from-0.tar staged/$f_hash

  # clean up test dir
  sudo rm -f "$DIR/target-dir/testfile"
}

teardown() {
  clean_tars 10
}

@test "bundle-add add bundle with bad hash in state dir" {
  # sanity check, the test is invalid if this exists now
  [ ! -f "$DIR/target-dir/testfile" ]
  run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

  check_lines "$output"
  [ "$status" -eq 0 ]
  # the corrected file should exist on the filesystem now
  [ -f "$DIR/target-dir/testfile" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
