#!/usr/bin/env bats

load "../../swupdlib"

dir_hash="24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af"

setup() {
  clean_test_dir
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.MoM.tar" Manifest.MoM Manifest.MoM.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.os-core.tar" Manifest.os-core Manifest.os-core.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.test-bundle.tar" Manifest.test-bundle Manifest.test-bundle.signed
  sudo chown root:root "$DIR/web-dir/10/staged/$dir_hash"
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/pack-test-bundle-from-0.tar" --exclude=staged/$dir_hash/* staged/$dir_hash
}

teardown() {
  pushd "$DIR/web-dir/10"
  rm *.tar
  popd
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/web-dir/10/staged/$dir_hash"
  sudo rmdir "$DIR/target-dir/usr/bin/"
}

@test "bundle-add add bundle containing a directory" {
  run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"

  echo "$output"
  [ "${lines[2]}" = "Attempting to download version string to memory" ]
  [ "${lines[3]}" = "Downloading packs..." ]
  [ "${lines[4]}" = "Downloading test-bundle pack for version 10" ]
  [ "${lines[5]}" = "Extracting pack." ]
  [ "${lines[6]}" = "Installing bundle(s) files..." ]
  [ "${lines[10]}" = "Bundle(s) installation done." ]
  [ -d "$DIR/target-dir/usr/bin" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
