BEGIN {
  checked_idx = 0
  checked_count = 0
}

# First, read all checked lines into an indexed array, later
# referenced when reading the file containing swupd output.
FILENAME ~ /lines-checked/ {
  tocheck[checked_count++] = $0
}

FILENAME ~ /lines-output/ {
  # An output line may match a regular expression from lines-checked, which
  # begins with the "REGEXP:" prefix. Otherwise, the line is checked as a
  # literal string.
  is_regexp = match(tocheck[checked_idx], /^REGEXP:/)

  if (is_regexp != 0) {
    regexp = substr(tocheck[checked_idx], RLENGTH + 1)
    if ($0 ~ regexp) {
      matched = 1
    } else {
      matched = 0
    }
  } else {
    # The actual output should equal the expected line
    if ($0 == tocheck[checked_idx]) {
      matched = 1
    } else {
      matched = 0
    }
  }

  if (matched == 1) {
    # We found a match, so move to the next entry in lines-checked.
    checked_idx++
  }
}

END {
  if (checked_idx < checked_count) {
    printf("\nMissing/incorrect lines detected, beginning with:\n")
    printf("\n   %s\n", tocheck[checked_idx])
    exit 1
  }
}
