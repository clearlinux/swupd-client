#!/usr/bin/env bats

load "../../swupdlib"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  create_manifest_tar 10 os-core
}

teardown() {
  clean_tars 10
}

@test "verify check missing directory" {
  run sudo sh -c "$SWUPD verify $SWUPD_OPTS"

  check_lines "$output"
  [ ! -d "$DIR/target-dir/usr/bin" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
