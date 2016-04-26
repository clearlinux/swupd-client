#!/usr/bin/env bats

load "../../swupdlib"

targetfile=e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3

setup() {
  clean_test_dir
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.MoM.tar" Manifest.MoM Manifest.MoM.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.os-core.tar" Manifest.os-core Manifest.os-core.signed
  sudo chown root:root "$DIR/target-dir/usr/lib/kernel/testfile"
  sudo chown root:root "$DIR/web-dir/10/files/$targetfile"
  tar -C "$DIR/web-dir/10/files" -cf "$DIR/web-dir/10/files/$targetfile.tar" $targetfile
}

teardown() {
  pushd "$DIR/web-dir/10"
  rm *.tar
  popd
  pushd "$DIR/web-dir/10/files"
  rm *.tar
  popd
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/web-dir/10/files/$targetfile"
  sudo rm "$DIR/target-dir/usr/lib/kernel/testfile"
  cp "$DIR/web-dir/10/files/$targetfile" "$DIR/target-dir/usr/lib/kernel/testfile"
}

@test "verify add missing boot file" {
  run sudo sh -c "$SWUPD verify --fix $SWUPD_OPTS"

  echo "$output"
  [ "${lines[5]}" = "Verifying version 10" ]
  [ "${lines[6]}" = "Attempting to download version string to memory" ]
  [ "${lines[7]}" = "Starting download of remaining update content. This may take a while..." ]
  [ "${lines[8]}" = "Finishing download of update content..." ]
  [ "${lines[9]}" = "Adding any missing files" ]
  [ "${lines[10]}" = "Fixing modified files" ]
  [ "${lines[11]}" = "Inspected 0 files" ]
  [ "${lines[12]}" = "  0 files were missing" ]
  [ "${lines[13]}" = "  0 files found which should be deleted" ]
  [ "${lines[15]}" = "Fix successful" ]
  [ -f "$DIR/target-dir/usr/lib/kernel/testfile" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
