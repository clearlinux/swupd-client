#!/usr/bin/env bats

load "../../swupdlib"

t1_hash="6c27df6efcd6fc401ff1bc67c970b83eef115f6473db4fb9d57e5de317eba96e"
t2_hash="e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 os-core
  create_manifest_tar 10 test-bundle1
  create_manifest_tar 10 test-bundle2
  chown_root "$DIR/web-dir/10/staged/$t1_hash"
  chown_root "$DIR/web-dir/10/staged/$t2_hash"
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/pack-test-bundle1-from-0.tar" --exclude=staged/$t1_hash/* staged/$t1_hash
  tar -C "$DIR/web-dir/10" -cf "$DIR/web-dir/10/pack-test-bundle2-from-0.tar" staged/$t2_hash
}

teardown() {
  clean_tars 10
  revert_chown_root "$DIR/web-dir/10/staged/$t1_hash"
  revert_chown_root "$DIR/web-dir/10/staged/$t2_hash"
  revert_chown_root "$DIR/web-dir/10/Manifest.os-core"
  revert_chown_root "$DIR/web-dir/10/Manifest.test-bundle1"
  revert_chown_root "$DIR/web-dir/10/Manifest.test-bundle2"
  sudo rmdir "$DIR/target-dir/usr/bin/"
  sudo rm "$DIR/target-dir/usr/foo"
}

# 8 combinations of rc to check, but skip all off
# bad name on/off (1)
# existing name on/off (2)
# new name on/off (4)
# Do it in order 1 4 3 2 5 7 6

@test "bundle-add returncodes part 1" {
  # Start with nothing installed
  # bad on existing off new off (1)
  run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS no-such-bundle"
  [ "$status" -ne 0 ]
  # bad off existing off new on (4)
  sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1" # Add it first time, OK
  # Now have bundle 1 installed
  # bad on existing on new off (3)
  run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS no-such-bundle test-bundle1" # Fail due to bad name
  [ "$status" -ne 0 ]
  # bad off existing on new off (2)
  sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1" # OK to add a second time
  # bad on existing off new on (5)
  run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS no-such-bundle test-bundle2"
  [ "$status" -ne 0 ]
  # bad on existing on new on (7)
  run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS no-such-bundle test-bundle1 test-bundle2" # Fail due to bad name
  [ "$status" -ne 0 ]
  # bad off existing on new on (6)
  sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1 test-bundle2" # OK to actually add


  [ -d "$DIR/target-dir/usr/bin" ]
  [ -f "$DIR/target-dir/usr/foo" ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
