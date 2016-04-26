#!/usr/bin/env bats

load "../../swupdlib"

os_release_file=1c1efd10467cbc1dddd8e63c3ef4d9099f36418ed5372d080a1bf0f03a49ab05

setup() {
  clean_test_dir
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.MoM.tar" Manifest.MoM Manifest.MoM.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.os-core.tar" Manifest.os-core Manifest.os-core.signed
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/Manifest.test-bundle1.tar" Manifest.test-bundle1 Manifest.test-bundle1.signed
  tar -C "$DIR/web-dir/20" -cf "$DIR/web-dir/20/Manifest.MoM.tar" Manifest.MoM Manifest.MoM.signed
  tar -C "$DIR/web-dir/20" -cf "$DIR/web-dir/20/Manifest.os-core.tar" Manifest.os-core Manifest.os-core.signed
  tar -C "$DIR/web-dir/20" -cf "$DIR/web-dir/20/Manifest.test-bundle2.tar" Manifest.test-bundle2 Manifest.test-bundle2.signed
  tar -C "$DIR/web-dir/30" -cf "$DIR/web-dir/30/Manifest.MoM.tar" Manifest.MoM Manifest.MoM.signed
  tar -C "$DIR/web-dir/30" -cf "$DIR/web-dir/30/Manifest.os-core.tar" Manifest.os-core Manifest.os-core.signed
  tar -C "$DIR/web-dir/30" -cf "$DIR/web-dir/30/Manifest.test-bundle2.tar" Manifest.test-bundle2 Manifest.test-bundle2.signed
  sudo chown root:root "$DIR/web-dir/30/staged/$os_release_file"
  tar -C "$DIR/web-dir/30" -cf "$DIR/web-dir/30/pack-os-core-from-20.tar" staged/$os_release_file
  cp "$DIR/target-dir/testfile" "$DIR"
}

teardown() {
  pushd "$DIR/web-dir/10"
  rm *.tar
  popd
  pushd "$DIR/web-dir/20"
  rm *.tar
  popd
  pushd "$DIR/web-dir/30"
  rm *.tar
  popd
  sudo chown $(ls -l "$DIR/test.bats" | awk '{ print $3 ":" $4 }') "$DIR/web-dir/30/staged/$os_release_file"
  sed -i 's/30/20/' "$DIR/target-dir/usr/lib/os-release"
  mv "$DIR/testfile" "$DIR/target-dir/"
}

@test "update where the newest version of a file was deleted" {
  run sudo sh -c "$SWUPD update $SWUPD_OPTS"

  echo "$output"
  [ "${lines[2]}" = "Attempting to download version string to memory" ]
  [ "${lines[3]}" = "Update started." ]
  [ "${lines[4]}" = "Querying server version." ]
  [ "${lines[5]}" = "Attempting to download version string to memory" ]
  [ "${lines[6]}" = "Preparing to update from 20 to 30" ]
  [ "${lines[7]}" = "Querying current manifest." ]
  [ "${lines[8]}" = "Querying server manifest." ]
  [ "${lines[9]}" = "Downloading os-core pack for version 30" ]
  [ "${lines[10]}" = "Extracting pack." ]
  [ "${lines[11]}" = "Downloading test-bundle2 pack for version 30" ]
  [ "${lines[12]}" = "Statistics for going from version 20 to version 30:" ]
  [ "${lines[13]}" = "    changed manifests : 2" ]
  [ "${lines[14]}" = "    new manifests     : 0" ]
  [ "${lines[15]}" = "    deleted manifests : 0" ]
  [ "${lines[16]}" = "    changed files     : 1" ]
  [ "${lines[17]}" = "    new files         : 0" ]
  [ "${lines[18]}" = "    deleted files     : 1" ]
  [ "${lines[19]}" = "Starting download of remaining update content. This may take a while..." ]
  [ "${lines[20]}" = "Finishing download of update content..." ]
  [ "${lines[21]}" = "Staging file content" ]
  [ "${lines[22]}" = "Update was applied." ]
  [ "${lines[26]}" = "Update successful. System updated from version 20 to version 30" ]
  [ ! -f "$DIR/target-dir/testfile" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
