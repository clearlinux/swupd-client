#!/usr/bin/env bats

load "../../swupdlib"

setup() {
  clean_state_dir
  clean_target_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  create_manifest_tar 20 MoM
  sign_manifest_mom 20
  create_manifest_tar 20 os-core
  set_os_release 10
  chown_root -R "$DIR/target-dir" "$DIR/web-dir"
  unpack_target_from_manifest 10 MoM
  unpack_target_from_manifest 10 os-core
  sudo tar -C "$DIR/web-dir/20" -cf "$DIR/web-dir/20/pack-os-core-from-10.tar" delta/ staged/
}

teardown() {
  revert_chown_root -R "$DIR/web-dir"
}

@test "update rename ghosted file" {
  run sudo sh -c "$SWUPD update $SWUPD_OPTS"

  [ $status -eq 0 ]
  check_lines "$output"

  # all the files should still exist, including the ghosted ones
  [ -e "$DIR/target-dir/bar" ]
  [ -e "$DIR/target-dir/foo" ]
  [ -e "$DIR/target-dir/bat" ]
  [ -e "$DIR/target-dir/baz" ]

}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
