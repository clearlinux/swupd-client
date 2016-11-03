#!/usr/bin/env bats

load "../../swupdlib"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle
  create_manifest_tar 100 MoM
  create_manifest_tar 100 test-bundle
  chown_root "$DIR/target-dir/foo"
}

teardown() {
  clean_tars 10
  clean_tars 100
  revert_chown_root "$DIR/target-dir/foo"
}

@test "update fullfile download skipped when hash verifies correctly" {
  run sudo sh -c "$SWUPD update $SWUPD_OPTS"

  check_lines "$output"

  # changed file (hash is the same, but version changed)
  [ -f "$DIR/target-dir/foo" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
