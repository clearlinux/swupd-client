#!/usr/bin/env bats

load "../../swupdlib"

@test "autocomplete exists" {
  [ -e $SRCDIR/swupd.bash ]
}

@test "autocomplete syntax" {
  bash $SRCDIR/swupd.bash
}

@test "autocomplete has autoupdate" {
  grep -q  '("autoupdate")' $SRCDIR/swupd.bash
}

@test "autocomplete has expected hashdump opts" {
  grep -q  'opts="-h --help -n --no-xattrs -p --path "' $SRCDIR/swupd.bash
}
# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
