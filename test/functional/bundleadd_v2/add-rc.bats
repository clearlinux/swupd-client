#!/usr/bin/env bats

load "../testlib"

setup() {

	create_test_environment "$TEST_NAME"
	create_bundle -n test-bundle1 -f /usr/bin/file1 "$TEST_NAME"
	create_bundle -n test-bundle2 -f /media/lib/file2 "$TEST_NAME"
	create_bundle -n test-bundle3 -f /media/lib/file3 "$TEST_NAME"
	create_bundle -n test-bundle4 -f /usr/file4 "$TEST_NAME"

}

teardown() {

	destroy_test_environment "$TEST_NAME"

}

# 8 combinations of rc to check, but skip all off
# bad name on/off (1)
# existing name on/off (2)
# new name on/off (4)
# Do it in order 1 4 3 2 5 7 6

@test "bundle-add returncodes part 1" {

	# Start with nothing installed
	[ -d "$TEST_NAME"/target-dir/usr/bin ]  # /usr/bin already exist because of os-core (installed by default)
	[ ! -f "$TEST_NAME"/target-dir/usr/bin/file1 ]
	[ ! -d "$TEST_NAME"/target-dir/media/lib ]
	[ ! -f "$TEST_NAME"/target-dir/media/lib/file2 ]
	[ ! -f "$TEST_NAME"/target-dir/media/lib/file3 ]
	[ ! -f "$TEST_NAME"/target-dir/usr/file4 ]

	# bad on existing off new off (1)
	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS no-such-bundle"
	# Fail: nothing is installed due to bad bundle name
	echo "Actual status: $status"
	[ "$status" -ne 0 ]

	# bad off existing off new on (4)
	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1"
	# Ok: Add it first time
	echo "Actual status: $status"
	[ "$status" -eq 0 ]
	[ -f "$TEST_NAME"/target-dir/usr/bin/file1 ]
	[ -d "$TEST_NAME"/target-dir/usr/bin ]

	# Now have bundle 1 installed
	# bad on existing on new off (3)
	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS no-such-bundle test-bundle1"
	# Fail: nothing is installed due to bad name and already installed
	echo "Actual status: $status"
	[ "$status" -ne 0 ]

	# bad off existing on new off (2)
	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1"
	# Fail: nothing is installed due to already being installed
	echo "Actual status: $status"
	[ "$status" -ne 0 ]

	# bad on existing off new on (5)
	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS no-such-bundle test-bundle2"
	# Fail: one bundle is installed, the other fails due to bad name
	echo "Actual status: $status"
	[ "$status" -ne 0 ]
	[ -f "$TEST_NAME"/target-dir/media/lib/file2 ]
	[ -d "$TEST_NAME"/target-dir/media/lib ]

	# bad on existing on new on (7)
	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS no-such-bundle test-bundle1 test-bundle3"
	# Fail: one bundle installed, other two fail due to bad name and already installed
	echo "Actual status: $status"
	[ "$status" -ne 0 ]
	[ -f "$TEST_NAME"/target-dir/media/lib/file3 ]
	[ -d "$TEST_NAME"/target-dir/media/lib ]

	# bad off existing on new on (6)
	run sudo sh -c "$SWUPD bundle-add $SWUPD_OPTS test-bundle1 test-bundle4"
	# Fail: one bundle installed, the other fail due to already installed
	[ -f "$TEST_NAME"/target-dir/usr/file4 ]
	[ -d "$TEST_NAME"/target-dir/usr ]

}
