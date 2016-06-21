#!/usr/bin/env bats

load "../../swupdlib"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  create_manifest_tar 10 test-bundle
}

teardown() {
  clean_tars 10
}

@test "bundle-add an already existing bundle" {
  run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

  echo "$output"
  [ "${lines[2]}" = "Attempting to download version string to memory" ]
  [ "${lines[3]}" = "bundle(s) already installed, exiting now" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
