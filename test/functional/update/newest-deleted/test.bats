#!/usr/bin/env bats

load "../../swupdlib"

os_release_file=1c1efd10467cbc1dddd8e63c3ef4d9099f36418ed5372d080a1bf0f03a49ab05

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle1
  create_manifest_tar 20 MoM
  create_manifest_tar 20 os-core
  create_manifest_tar 20 test-bundle2
  create_manifest_tar 30 MoM
  create_manifest_tar 30 os-core
  create_manifest_tar 30 test-bundle2
  chown_root "$DIR/web-dir/30/staged/$os_release_file"
  tar -C "$DIR/web-dir/30" -cf "$DIR/web-dir/30/pack-os-core-from-20.tar" staged/$os_release_file
  cp "$DIR/target-dir/testfile" "$DIR"
}

teardown() {
  clean_tars 10
  clean_tars 20
  clean_tars 30
  revert_chown_root "$DIR/web-dir/30/staged/$os_release_file"
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
  ignore_sigverify_error 8
  [ "${lines[8]}" = "Querying server manifest." ]
  ignore_sigverify_error 9
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
