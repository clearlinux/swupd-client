#!/usr/bin/env bats

load "../../swupdlib"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 test-bundle
}

teardown() {
  clean_tars 10
}

@test "bundle-add add bundle with invalid number of files in manifest" {
  run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

  # it will retry a number of times, so takes about 70 seconds
  [ "$status" -gt 0 ]
  check_lines "$output"
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
