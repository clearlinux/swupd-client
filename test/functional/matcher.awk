BEGIN {
  checked_idx = 0
  checked_count = 0
  early_exit = 0
}

# There are several lines that can be ignored for all tests.
FILENAME ~ /ignore-list/ {
  toignore[$0]++
}

# Checks if LINE matches any pattern in the "toignore" array. Returns the first
# matching pattern if true, 0 if false.
function is_ignored(line) {
  for (pattern in toignore) {
    if (line ~ pattern) {
      return pattern
    }
  }
  return 0
}

# Read all checked lines into an indexed array, later referenced when reading
# the file containing swupd output.
FILENAME ~ /lines-checked/ {
  # Validate that none of these lines match the patterns from the ignore list,
  # since the comparison of lines-checked to lines-output requires it.
  result = is_ignored($0)
  if (result) {
    bad_lines[bad_count++] = sprintf("'%s' matches '%s'", $0, result)
    next
  }

  tocheck[checked_count++] = $0
}

FILENAME ~ /lines-output/ {
  if (0 in bad_lines) {
    exit 2
  }

  # Skip to the next line if the current line matches any patterns from the
  # global ignore list.
  if (is_ignored($0)) {
    next
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
  if (0 in bad_lines) {
    printf("\nDetected ignored lines in lines-checked:\n\n")
    for (i in bad_lines) {
      printf("    %s\n", bad_lines[i])
    }
    printf("\n")
    exit 2
  }

  if (early_exit == 0 && checked_idx < checked_count) {
    printf("\nDetected missing line(s) in the output, beginning with:\n")
    printf("\n   %s\n", tocheck[checked_idx])
    exit 1
  }
}


# vi: ft=awk sw=2 sts=2 et tw=80
