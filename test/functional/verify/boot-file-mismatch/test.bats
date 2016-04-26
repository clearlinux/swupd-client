#!/usr/bin/env bats

load "../../swupdlib"

setup() {
  clean_test_dir
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.MoM.tar" Manifest.MoM Manifest.MoM.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.os-core.tar" Manifest.os-core Manifest.os-core.signed
  sudo chown root:root "$DIR/target-dir/usr/lib/kernel/testfile"
}

teardown() {
  pushd "$DIR/web-dir/10"
  rm *.tar
  popd
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/target-dir/usr/lib/kernel/testfile"
}

@test "verify check incorrect boot file" {
  run sudo sh -c "$SWUPD verify $SWUPD_OPTS"

  echo "$output"
  [ "${lines[2]}" = "Verifying version 10" ]
  [ "${lines[3]}" = "Attempting to download version string to memory" ]
  [ "${lines[4]}" = "Verifying files" ]
  [ "${lines[6]}" = "Inspected 1 files" ]
  [ "${lines[7]}" = "  1 files did not match" ]
  [ "${lines[8]}" = "Verify successful" ]
  [ -f "$DIR/target-dir/usr/lib/kernel/testfile" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
