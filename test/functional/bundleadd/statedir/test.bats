#!/usr/bin/env bats

load "../../swupdlib"

setup() {
  clean_test_dir
  create_manifest_tar 10 MoM
  sign_manifest_mom 10
  create_manifest_tar 10 test-bundle
}

teardown() {
  clean_tars 10
}

@test "ensure statedir has correct ownership and permissions" {
  sudo sh -c "mkdir -p  $STATE_DIR; chown 1 $STATE_DIR ; chmod ugo+rx $STATE_DIR"
  run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle"
  [ "$status" -eq 0 ]
  check_lines "$output"
  [ "$(stat --printf %u.%a $STATE_DIR)" = 0.700 ]
  [ "$(sudo stat --printf %u.%a $STATE_DIR/staged)" = 0.700 ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
