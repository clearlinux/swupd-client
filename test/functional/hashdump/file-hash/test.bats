#!/usr/bin/env bats

load "../../swupdlib"

setup() {
  chown_root "$DIR/target-dir/test-hash"
}

teardown() {
  revert_chown_root "$DIR/target-dir/test-hash"
}

@test "hashdump with prefix" {
  run sudo sh -c "$SWUPD hashdump --basepath=$DIR/target-dir /test-hash"

  echo "$output"
  [ "${lines[2]}" = "b03e11e3307de4d4f3f545c8a955c2208507dbc1927e9434d1da42917712c15b" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
