#!/usr/bin/env bats

load "../../swupdlib"

targetfile=ffbe308091534f3b47b876b241166c4c6d0665204977274093f74851380d49b5

setup() {
  clean_test_dir
  mkdir -p "$DIR/state/staged"
  sudo cp "$DIR/$targetfile" "$DIR/state/staged/$targetfile"
  chown_root -R "$DIR/state"
  sudo chmod -R 0700 "$DIR/state"
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  create_manifest_tar 100 MoM
  sign_manifest_mom 100
  create_manifest_tar 100 os-core
  create_fullfile_tar 100 $targetfile
}

teardown() {
  clean_tars 10
  clean_tars 100
  clean_tars 100 files
  revert_chown_root "$DIR/web-dir/100/files/$targetfile"
  sudo rm "$DIR/target-dir/usr/testfile"
}

@test "update full file with bad hash in state dir" {
  # sanity check that the file does not exist in the target-dir before swupd
  # puts it there
  [ ! -f "$DIR/target-dir/usr/testfile" ]

  run sudo sh -c "$SWUPD update $SWUPD_OPTS"

  # swupd should succeed
  [ "$status" -eq 0 ]
  # no errors are printed for a mismatched hash
  check_lines "$output"
  # the file exists on the filesystem (it does not exist before the test)
  [ -f "$DIR/target-dir/usr/testfile" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
