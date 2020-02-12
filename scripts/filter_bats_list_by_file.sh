#!/bin/bash
# Filter a list of jobs
# Parameter 1: Job number
# Parameter 2: Number of jobs
#
# To test if any test is missing:
# for i in $(seq $NUM_JOBS); do
#    ./scripts/filter_bats_list_by_file.sh $i $NUM_JOBS >> list;
# done

JOB_NUM=$(($1 - 1))
NUM_JOBS=$2
TOTAL_TESTS="$(find test/functional/ -name *.bats | wc -l)"
COUNT="$((TOTAL_TESTS/(NUM_JOBS - 1)))"
START="$((JOB_NUM * COUNT + 1))"
END="$(((JOB_NUM + 1) * COUNT))"
find test/functional/ -name *.bats | sed -n "${START},${END}p" | tr '\n' ' '
