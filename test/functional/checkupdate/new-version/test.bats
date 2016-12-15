#!/usr/bin/env bats

load "../../swupdlib"

@test "check-update with a new version available" {
  run sudo sh -c "$SWUPD check-update $SWUPD_OPTS_NO_CERT"

  [ "$status" -eq 0 ]
  check_lines "$output"
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
