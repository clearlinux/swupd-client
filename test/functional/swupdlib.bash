# NOTE: source this file from a *.bats file

# The location of the swupd_* binaries
export SRCDIR="$BATS_TEST_DIRNAME/../../../../"

export SWUPD="$SRCDIR/swupd"

export DIR="$BATS_TEST_DIRNAME"

export STATE_DIR="$BATS_TEST_DIRNAME/state"

export SWUPD_OPTS="-S $STATE_DIR -p $DIR/target-dir -F staging -u file://$DIR/web-dir"

clean_test_dir() {
  sudo rm -rf "$STATE_DIR"
}


# vi: ft=sh ts=8 sw=2 sts=2 et tw=80
