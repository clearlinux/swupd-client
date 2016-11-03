#!/usr/bin/env bats

load "../../swupdlib"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  create_manifest_tar 10 os-core
  chown_root "$DIR/target-dir/usr/lib/kernel/testfile"
}

teardown() {
  clean_tars 10
  revert_chown_root "$DIR/target-dir/usr/lib/kernel/testfile"
}

@test "verify check incorrect boot file" {
  run sudo sh -c "$SWUPD verify $SWUPD_OPTS"

  check_lines "$output"
  [ -f "$DIR/target-dir/usr/lib/kernel/testfile" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
