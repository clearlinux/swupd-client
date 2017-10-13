#!/usr/bin/env bats

load "../../swupdlib"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  chown_root -R "$DIR/target-dir"
}

teardown() {
  clean_tars 10
  sudo cp -r "$DIR/target-dir.bak/usr" "$DIR/target-dir/"
}

@test "verify ghosted file not added during --picky" {
  run sudo sh -c "$SWUPD verify --fix --picky $SWUPD_OPTS"

  [ $status -eq 0 ]
  check_lines "$output"
  # this should not exist at the end, despite being marked in the Manifest as
  # "ghosted". In this case ghosted files should be ignored.
  [ ! -f "$DIR/target-dir/usr/foo" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
