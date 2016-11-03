#!/usr/bin/env bats

load "../../swupdlib"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  create_manifest_tar 10 os-core
  rm "$DIR/target-dir/testdir/.gitignore"
}

teardown() {
  clean_tars 10
  mkdir -p "$DIR/target-dir/testdir"
  touch "$DIR/target-dir/testdir/.gitignore"
}

@test "verify delete already removed directory" {
  run sudo sh -c "$SWUPD verify --fix $SWUPD_OPTS"

  check_lines "$output"
  [ ! -d "$DIR/target-dir/testdir" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
