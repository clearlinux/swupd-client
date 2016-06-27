#!/usr/bin/env bats

load "../../swupdlib"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle
  create_manifest_tar 10 test-bundle2
}

teardown() {
  clean_tars 10
}

@test "bundle-remove remove bundle containing a file" {
  run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle"

  echo "$output"
  [ "${lines[2]}" = "Attempting to download version string to memory" ]
  ignore_sigverify_error 3
  [ "${lines[3]}" = "Error: bundle requested to be removed is required by other installed bundles" ]
  [ "${lines[4]}" = "Error: Bundle remove failed" ]
  [ -f "$DIR/target-dir/test-file" ]
  [ -f "$DIR/target-dir/test-file2" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
