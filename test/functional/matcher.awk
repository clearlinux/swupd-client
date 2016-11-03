BEGIN {
  checked_idx = 0
  checked_count = 0
  early_exit = 0
}

# First, read all checked lines into an indexed array, later
# referenced when reading the file containing swupd output.
FILENAME ~ /lines-checked/ {
  tocheck[checked_count++] = $0
}

# There are several lines that can be ignored for all tests.
FILENAME ~ /ignore-list/ {
  toignore[$0]++
}

FILENAME ~ /lines-output/ {
  # Skip to the next line if the current line matches any patterns from the
  # global ignore list.
  for (pattern in toignore) {
    if ($0 ~ pattern) {
      next
    }
  }

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
    if ($0 == tocheck[checked_idx]) {
      matched = 1
    } else {
      matched = 0
    }
  }

  if (matched == 1) {
    # We found a match, so move to the next entry in lines-checked.
    checked_idx++
  } else {
    # When a line is not checked or not ignored, it's an error
    early_exit = 1
    exit 1
  }
}

END {
  if (early_exit == 0 && checked_idx < checked_count) {
    printf("\nDetected missing line(s) in the output, beginning with:\n")
    printf("\n   %s\n", tocheck[checked_idx])
    exit 1
  }
}
