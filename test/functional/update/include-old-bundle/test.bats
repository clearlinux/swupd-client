#!/usr/bin/env bats

load "../../swupdlib"

f1=491685caf9cf95df0f721254748df4717b4159513a3e0170ff5fa404dccc32e7
f2=a96e0b959874854750e8e08372e62c4d1821c5e0106694365396d02c363ada50
f3=826e0a73bdb6ca4863842b07ead83a55b478a950fefd2b1832b30046ce1c1550

setup() {
  clean_test_dir
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.MoM.tar" Manifest.MoM Manifest.MoM.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.os-core.tar" Manifest.os-core Manifest.os-core.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.test-bundle1.tar" Manifest.test-bundle1 Manifest.test-bundle1.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.test-bundle2.tar" Manifest.test-bundle2 Manifest.test-bundle2.signed
  sudo chown root:root "$DIR/web-dir/10/files/$f1"
  tar -C "$DIR/web-dir/10/files" -cf "$DIR/web-dir/10/files/$f1.tar" $f1

  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/Manifest.MoM.tar" Manifest.MoM Manifest.MoM.signed
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/Manifest.os-core.tar" Manifest.os-core Manifest.os-core.signed
  tar -C "$DIR/web-dir/100" -cf "$DIR/web-dir/100/Manifest.test-bundle1.tar" Manifest.test-bundle1 Manifest.test-bundle1.signed
  sudo chown root:root "$DIR/web-dir/100/files/$f2"
  sudo chown root:root "$DIR/web-dir/100/files/$f3"
  tar -C "$DIR/web-dir/100/files" -cf "$DIR/web-dir/100/files/$f2.tar" $f2
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
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/web-dir/100/files/$f2"
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/web-dir/100/files/$f3"
  sudo rm "$DIR/target-dir/os-core"
  sudo rm "$DIR/target-dir/test-bundle1"
  sudo rm "$DIR/target-dir/test-bundle2"
}

@test "update include a bundle from an older release" {
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
  [ "${lines[10]}" = "Downloading test-bundle1 pack for version 100" ]
  [ "${lines[11]}" = "Statistics for going from version 10 to version 100:" ]
  [ "${lines[12]}" = "    changed manifests : 2" ]
  [ "${lines[13]}" = "    new manifests     : 0" ]
  [ "${lines[14]}" = "    deleted manifests : 0" ]
  [ "${lines[15]}" = "    changed files     : 2" ]
# FIXME not detecting new files from includes
  [ "${lines[16]}" = "    new files         : 0" ]
  [ "${lines[17]}" = "    deleted files     : 0" ]
  [ "${lines[18]}" = "Starting download of remaining update content. This may take a while..." ]
  [ "${lines[19]}" = "Finishing download of update content..." ]
  [ "${lines[20]}" = "Staging file content" ]
  [ "${lines[21]}" = "Update was applied." ]
  [ "${lines[25]}" = "Update successful. System updated from version 10 to version 100" ]
  [ -f "$DIR/target-dir/os-core" ]
  [ -f "$DIR/target-dir/test-bundle1" ]
  [ -f "$DIR/target-dir/test-bundle2" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
