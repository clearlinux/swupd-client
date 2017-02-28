#!/usr/bin/env bats

load "../../swupdlib"

@test "bundle-remove ensure bundle name is passed as an option" {
  run sudo sh -c "$SWUPD bundle-remove $SWUPD_OPTS"

  # errcode 15 is EINVALID_OPTION
  [ $status -eq 15 ]
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
