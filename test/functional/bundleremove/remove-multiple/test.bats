#!/usr/bin/env bats

load "../../swupdlib"

setup() {
  clean_test_dir
  mkdir -p "$DIR/target-dir/usr/share/clear/bundles/"
  touch "$DIR/target-dir/usr/share/clear/bundles/test-bundle1"
  touch "$DIR/target-dir/test-file1"
  touch "$DIR/target-dir/usr/share/clear/bundles/test-bundle2"
  touch "$DIR/target-dir/test-file2"
  touch "$DIR/target-dir/usr/share/clear/bundles/test-bundle3"
  touch "$DIR/target-dir/test-file3"
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle1
  create_manifest_tar 10 test-bundle2
  create_manifest_tar 10 test-bundle3
}

teardown() {
  clean_tars 10
}

@test "bundle-remove remove multiple bundles each containing a file" {
  run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle1 test-bundle2"

  [ "$status" -eq 0 ]
  check_lines "$output"
  [ ! -f "$DIR/target-dir/test-file1" ] && [ ! -f "$DIR/target-dir/test-file2" ] && [ -f "$DIR/target-dir/test-file3" ]
}

@test "bundle-remove remove multiple bundles one exist and one that dont" {
  run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle3 fake-bundle"

  [ ! -f "$DIR/target-dir/test-file3" ] && [ $status -eq 3 ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
