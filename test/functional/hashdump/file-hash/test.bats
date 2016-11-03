#!/usr/bin/env bats

load "../../swupdlib"

setup() {
  chown_root "$DIR/target-dir/test-hash"
}

teardown() {
  revert_chown_root "$DIR/target-dir/test-hash"
}

@test "hashdump with prefix" {
  run sudo sh -c "$SWUPD hashdump --path=$DIR/target-dir /test-hash"

  check_lines "$output"
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
