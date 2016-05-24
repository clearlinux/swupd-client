#!/usr/bin/env bats

load "../../swupdlib"

t1_hash="e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3"

setup() {
  clean_test_dir
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.MoM.tar" Manifest.MoM Manifest.MoM.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.os-core.tar" Manifest.os-core Manifest.os-core.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.test-bundle.tar" Manifest.test-bundle Manifest.test-bundle.signed
  sudo chown root:root "$DIR/web-dir/10/files/$t1_hash"
  tar -C "$DIR/web-dir/10/files" -cf "$DIR/web-dir/10/files/$t1_hash.tar" $t1_hash --exclude=$t1_hash/*
}

teardown() {
  pushd "$DIR/web-dir/10"
  rm *.tar
  popd
  pushd "$DIR/web-dir/10/files"
  rm *.tar
  popd
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/web-dir/10/files/$t1_hash"
  sudo rm "$DIR/target-dir/foo"
}

@test "bundle-add verify_fix_path support" {
  run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

  echo "$output"
  [ "${lines[2]}" = "Attempting to download version string to memory" ]
  [ "${lines[3]}" = "Downloading packs..." ]
  [ "${lines[4]}" = "Downloading test-bundle pack for version 10" ]
  [ "${lines[5]}" = "Installing bundle(s) files..." ]
  [ "${lines[6]}" = "Path /foo is missing on the file system" ]
  [ "${lines[10]}" = "Bundle(s) installation done." ]
  [ -f "$DIR/target-dir/foo" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
