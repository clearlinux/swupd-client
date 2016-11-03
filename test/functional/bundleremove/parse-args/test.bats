#!/usr/bin/env bats

load "../../swupdlib"

@test "bundle-remove ensure bundle name is passed as an option" {
  run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS"

  # errcode 15 is EINVALID_OPTION
  [ $status -eq 15 ]
}

@test "bundle-remove ensure only 1 bundle can be removed at a time" {
  run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS test-bundle test-bundle2"

  # errcode 15 is EINVALID_OPTION
  [ $status -eq 15 ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
