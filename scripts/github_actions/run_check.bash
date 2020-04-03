#!/bin/bash
set -e

JOB_NUMBER="$1"
NUM_JOBS="$2"

FILES=$(scripts/github_actions/filter_bats_list.bash "$JOB_NUMBER" "$NUM_JOBS")
NUM_FILES=$(echo "$FILES" | tr ' ' '\n' | wc -l)
echo "Running $NUM_FILES tests"
# shellcheck disable=SC2116
# shellcheck disable=SC2086
# Weird, but we really need this
env TESTS="$(echo $FILES)" make -e -j2 check

echo "$FILES" > job-"$JOB_NUMBER"
