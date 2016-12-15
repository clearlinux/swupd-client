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

@test "search with non existant file, specifying full path" {
  run sudo sh -c "$SWUPD search $SWUPD_OPTS /usr/lib64/test-lib100"

  echo "$output" | grep -q 'Search term not found.'
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
