#!/usr/bin/env bats

load "../../swupdlib"

f1=24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af
f2=cde33514c151abb2b01448be290ab1d5212952571d15e3533cadc15ff82f2cd5
f3=c2fb5e545287e632581b82e371de5d4aac2c7eb50d7f357f3b66fb0fdca71dd9

setup() {
  clean_test_dir
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.MoM.tar" Manifest.MoM Manifest.MoM.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.os-core.tar" Manifest.os-core Manifest.os-core.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.test-bundle.tar" Manifest.test-bundle Manifest.test-bundle.signed
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/Manifest.MoM.tar" Manifest.MoM Manifest.MoM.signed
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/Manifest.test-bundle.tar" Manifest.test-bundle Manifest.test-bundle.signed
  sudo chown root:root "$DIR/web-dir/10/files/$f1"
  sudo chown root:root "$DIR/web-dir/10/files/$f2"
  sudo chown root:root "$DIR/web-dir/100/files/$f3"
  tar -C "$DIR/web-dir/10/files" -cf "$DIR/web-dir/10/files/$f1.tar" $f1 --exclude=$f1/*
  tar -C "$DIR/web-dir/10/files" -cf "$DIR/web-dir/10/files/$f2.tar" $f2 --exclude=$f2/*
  tar -C "$DIR/web-dir/100/files" -cf "$DIR/web-dir/100/files/$f3.tar" $f3
}

teardown() {
  pushd "$DIR/web-dir/10"
  rm *.tar
  popd
  pushd "$DIR/web-dir/100"
  rm *.tar
  popd
  pushd "$DIR/web-dir/10/files"
  rm *.tar
  popd
  pushd "$DIR/web-dir/100/files"
  rm *.tar
  popd
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/web-dir/10/files/$f1"
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/web-dir/10/files/$f2"
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/web-dir/100/files/$f3"
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/target-dir/usr"
  sudo rm "$DIR/target-dir/usr/bin/foo"
  sudo rmdir "$DIR/target-dir/usr/bin"
}

@test "update verify_fix_path corrects hash mismatch" {
  run sudo sh -c "$SWUPD update $SWUPD_OPTS"

  echo "$output"
  [ "${lines[2]}" = "Attempting to download version string to memory" ]
  [ "${lines[3]}" = "Update started." ]
  [ "${lines[4]}" = "Querying server version." ]
  [ "${lines[5]}" = "Attempting to download version string to memory" ]
  [ "${lines[6]}" = "Preparing to update from 10 to 100" ]
  [ "${lines[7]}" = "Querying current manifest." ]
  [ "${lines[8]}" = "Querying server manifest." ]
  [ "${lines[9]}" = "Downloading test-bundle pack for version 100" ]
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
  [ "${lines[21]}" = "Hash did not match for path : /usr" ]
  [ "${lines[22]}" = "Path /usr/bin is missing on the file system" ]
  [ "${lines[23]}" = "Update was applied." ]
  [ "${lines[27]}" = "Update successful. System updated from version 10 to version 100" ]
  [ -d "$DIR/target-dir/usr/bin" ]
  [ -f "$DIR/target-dir/usr/bin/foo" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
