#!/usr/bin/env bats

# Author: Otavio Pontes
# Email: otavio.pontes@intel.com

load "../testlib"

test_setup() {

	# do nothing
	return

}

test_teardown() {

	# do nothing
	return

}

@test "USA010: Check if there are any duplicated test ID" {
	num_tests=$(list_tests --all | wc -l)
	num_unique_tests=$(list_tests --all | cut -d ":" -f 1 | sort -u | wc -l)

	if [ "$num_tests" -ne "$num_unique_tests" ]; then
		dup=$(expr "$num_tests" - "$num_unique_tests")
		print "Found $dup tests with duplicated IDS. Run list_tests --all to check that."
		exit 1
	fi
}
