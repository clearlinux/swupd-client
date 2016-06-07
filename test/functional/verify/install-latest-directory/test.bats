#!/usr/bin/env bats

load "../../swupdlib"

targetfile=24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af

setup() {
  clean_test_dir
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/Manifest.MoM.tar" Manifest.MoM Manifest.MoM.signed
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/Manifest.os-core.tar" Manifest.os-core Manifest.os-core.signed
  sudo chown root:root "$DIR/web-dir/100/staged/$targetfile"
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/pack-os-core-from-0.tar" --exclude=staged/$targetfile/* staged/$targetfile
}

teardown() {
  pushd "$DIR/web-dir/100"
  rm *.tar
  popd
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/web-dir/100/staged/$targetfile"
  sudo rmdir "$DIR/target-dir/usr/bin/"
}

@test "verify install a directory using latest" {
  run sudo sh -c "$SWUPD verify $SWUPD_OPTS --install -m latest"

  echo "$output"
  [ "${lines[5]}" = "Attempting to download version string to memory" ]
  [ "${lines[6]}" = "Verifying version 100" ]
  [ "${lines[7]}" = "Attempting to download version string to memory" ]
  [ "${lines[8]}" = "Downloading os-core pack for version 100" ]
  [ "${lines[9]}" = "Extracting pack." ]
  [ "${lines[10]}" = "Adding any missing files" ]
  [ "${lines[11]}" = "Inspected 1 files" ]
  [ "${lines[12]}" = "  1 files were missing" ]
  [ "${lines[13]}" = "    1 of 1 missing files were replaced" ]
  [ "${lines[14]}" = "    0 of 1 missing files were not replaced" ]
  [ "${lines[16]}" = "Fix successful" ]
  [ -d "$DIR/target-dir/usr/bin" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
