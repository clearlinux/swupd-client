#!/usr/bin/env bats

load "../../swupdlib"

targetfile=24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af

setup() {
  clean_test_dir
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.MoM.tar" Manifest.MoM Manifest.MoM.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.os-core.tar" Manifest.os-core Manifest.os-core.signed
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/Manifest.MoM.tar" Manifest.MoM Manifest.MoM.signed
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/Manifest.os-core.tar" Manifest.os-core Manifest.os-core.signed
  sudo chown root:root "$DIR/web-dir/100/staged/$targetfile"
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/pack-os-core-from-10.tar" --exclude=staged/$targetfile/* staged/$targetfile
}

teardown() {
  pushd "$DIR/web-dir/10"
  rm *.tar
  popd
  pushd "$DIR/web-dir/100"
  rm *.tar
  popd
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/web-dir/100/staged/$targetfile"
  sudo rmdir "$DIR/target-dir/usr/bin"
}

@test "update using a pack" {
  run sudo sh -c "$SWUPD update $SWUPD_OPTS"

  echo "$output"
  [ "${lines[2]}" = "Attempting to download version string to memory" ]
  [ "${lines[3]}" = "Update started." ]
  [ "${lines[4]}" = "Querying server version." ]
  [ "${lines[5]}" = "Attempting to download version string to memory" ]
  [ "${lines[6]}" = "Preparing to update from 10 to 100" ]
  [ "${lines[7]}" = "Querying current manifest." ]
  [ "${lines[8]}" = "Querying server manifest." ]
  [ "${lines[9]}" = "Downloading os-core pack for version 100" ]
  [ "${lines[10]}" = "Extracting pack." ]
  [ "${lines[11]}" = "Statistics for going from version 10 to version 100:" ]
  [ "${lines[12]}" = "    changed manifests : 1" ]
  [ "${lines[13]}" = "    new manifests     : 0" ]
  [ "${lines[14]}" = "    deleted manifests : 0" ]
  [ "${lines[15]}" = "    changed files     : 1" ]
  [ "${lines[16]}" = "    new files         : 0" ]
  [ "${lines[17]}" = "    deleted files     : 0" ]
  [ "${lines[18]}" = "Starting download of remaining update content. This may take a while..." ]
  [ "${lines[19]}" = "Finishing download of update content..." ]
  [ "${lines[20]}" = "Staging file content" ]
  [ "${lines[21]}" = "Update was applied." ]
  [ "${lines[25]}" = "Update successful. System updated from version 10 to version 100" ]
  [ -d "$DIR/target-dir/usr/bin" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
