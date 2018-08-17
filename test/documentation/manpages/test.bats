#!/usr/bin/env bats

setup() {
  # Ensure that we have the conversion program
  type -t rst2man.py > /dev/null
  # Change to the toplevel git directory
  type -t git >/dev/null || skip
  cd "$(git rev-parse --show-toplevel)"
}

@test "Check manual pages are up to date" {
  for rst in docs/*.[1-9].rst
  do
    [ -f "$rst" ] || continue	# Could use shopt -s nullglob
    echo "$rst"
    rst2man.py "$rst" | cmp  "${rst%.rst}" -
  done
}

@test "Check manual pages are clean" {
  for nroff in docs/*.[1-9]
  do
    [ -f "$nroff" ] || continue
    echo "$nroff"
    git diff --exit-code HEAD "$nroff"
  done
}

# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
