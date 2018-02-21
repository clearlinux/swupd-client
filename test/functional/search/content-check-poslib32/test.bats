#!/usr/bin/env bats

load "../../swupdlib"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 test-bundle
  create_manifest_tar 10 os-core
}

teardown() {
  clean_tars 10
}

@test "search for a library in lib32" {
  run sudo sh -c "$SWUPD search $SWUPD_OPTS -l test-lib32"

  [ "$status" -eq 0 ]
  echo "$output" | grep -q "Bundle test-bundle"
  echo "$output" | grep -q "/usr/lib/test-lib32"
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
