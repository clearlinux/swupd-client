#!/usr/bin/env bats

load "../../swupdlib"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
}

teardown() {
  clean_tars 10
}

@test "bundle-list all bundles" {
  run sudo sh -c "$SWUPD bundle-list --all"

  check_lines "$output"
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
