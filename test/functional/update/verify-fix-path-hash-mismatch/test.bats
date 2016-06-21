#!/usr/bin/env bats

load "../../swupdlib"

f1=24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af
f2=cde33514c151abb2b01448be290ab1d5212952571d15e3533cadc15ff82f2cd5
f3=520f83440d3dddc25ad09ca858b9c669245f82d3181a45cdfe793aac9dd1fb15

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle
  create_manifest_tar 100 MoM
  create_manifest_tar 100 test-bundle
  create_fullfile_tar 10 $f1
  create_fullfile_tar 10 $f2
  create_fullfile_tar 100 $f3
}

teardown() {
  clean_tars 10
  clean_tars 100
  clean_tars 10 files
  clean_tars 100 files
  revert_chown_root "$DIR/web-dir/10/files/$f1"
  revert_chown_root "$DIR/web-dir/10/files/$f2"
  revert_chown_root "$DIR/web-dir/100/files/$f3"
  revert_chown_root "$DIR/target-dir/usr"
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
