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
  clean_tars 10 files
  sudo cp -r "$DIR/target-dir.bak/foo" "$DIR/target-dir"
}

@test "verify ghosted file skipped" {
  run sudo sh -c "$SWUPD verify --fix $SWUPD_OPTS"

  [ $status -eq 0 ]
  check_lines "$output"
  # this should exist at the end, despite being marked as "ghosted" in the
  # Manifest and treated as deleted for parts of swupd-client.
  [ -f "$DIR/target-dir/foo" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
