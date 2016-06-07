#!/usr/bin/env bats

load "../../swupdlib"

targetfile=24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af

setup() {
  clean_test_dir
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.MoM.tar" Manifest.MoM Manifest.MoM.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.os-core.tar" Manifest.os-core Manifest.os-core.signed
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/Manifest.MoM.tar" Manifest.MoM Manifest.MoM.signed
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/Manifest.os-core.tar" Manifest.os-core Manifest.os-core.signed
  sudo chown root:root "$DIR/web-dir/100/files/$targetfile"
  tar -C "$DIR/web-dir/100/files" -cf "$DIR/web-dir/100/files/$targetfile.tar" --exclude=$targetfile/* $targetfile
}

teardown() {
  pushd "$DIR/web-dir/10"
  rm *.tar
  popd
  pushd "$DIR/web-dir/100"
  rm *.tar
  popd
  pushd "$DIR/web-dir/100/files"
  rm *.tar
  popd
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/web-dir/100/files/$targetfile"
  sudo rmdir "$DIR/target-dir/usr/bin/"
}

@test "update use full file" {
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
  [ "${lines[10]}" = "Statistics for going from version 10 to version 100:" ]
  [ "${lines[11]}" = "    changed manifests : 1" ]
  [ "${lines[12]}" = "    new manifests     : 0" ]
  [ "${lines[13]}" = "    deleted manifests : 0" ]
  [ "${lines[14]}" = "    changed files     : 1" ]
  [ "${lines[15]}" = "    new files         : 0" ]
  [ "${lines[16]}" = "    deleted files     : 0" ]
  [ "${lines[17]}" = "Starting download of remaining update content. This may take a while..." ]
  [ "${lines[18]}" = "Finishing download of update content..." ]
  [ "${lines[19]}" = "Staging file content" ]
  [ "${lines[20]}" = "Update was applied." ]
  [ "${lines[24]}" = "Update successful. System updated from version 10 to version 100" ]
  [ -d "$DIR/target-dir/usr/bin" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
