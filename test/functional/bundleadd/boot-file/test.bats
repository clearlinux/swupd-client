#!/usr/bin/env bats

load "../../swupdlib"

f_hash="e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3"

setup() {
  clean_test_dir
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.MoM.tar" Manifest.MoM Manifest.MoM.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.test-bundle.tar" Manifest.test-bundle Manifest.test-bundle.signed
  sudo chown root:root "$DIR/web-dir/10/staged/$f_hash"
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/pack-test-bundle-from-0.tar" staged/$f_hash
}

teardown() {
  pushd "$DIR/web-dir/10"
  rm *.tar
  popd
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/web-dir/10/staged/$f_hash"
  sudo rm "$DIR/target-dir/usr/lib/kernel/testfile"
  sudo rm "$DIR/target-dir/usr/share/clear/bundles/test-bundle"
}

@test "bundle-add add bundle containing boot file" {
  run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

  echo "$output"
  [ "${lines[2]}" = "Attempting to download version string to memory" ]
  [ "${lines[3]}" = "Downloading required packs..." ]
  [ "${lines[4]}" = "Downloading test-bundle pack for version 10" ]
  [ "${lines[5]}" = "Extracting pack." ]
  [ "${lines[6]}" = "Installing bundle(s) files..." ]
  [ "${lines[11]}" = "Bundle(s) installation done." ]
  [ -f "$DIR/target-dir/usr/lib/kernel/testfile" ]
  [ -f "$DIR/target-dir/usr/share/clear/bundles/test-bundle" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
