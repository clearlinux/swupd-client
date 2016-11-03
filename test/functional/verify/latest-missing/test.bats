#!/usr/bin/env bats

load "../../swupdlib"

@test "verify install using latest with missing version file on server" {
  run sudo sh -c "$SWUPD verify $SWUPD_OPTS --install -m latest"

  check_lines "$output"
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
