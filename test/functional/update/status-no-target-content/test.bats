#!/usr/bin/env bats

load "../../swupdlib"

@test "update --status with no target content" {
  run sudo sh -c "$SWUPD update $SWUPD_OPTS --status"

  check_lines "$output"
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
