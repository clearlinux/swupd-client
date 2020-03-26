#!/bin/bash
# Filter a list of jobs
# Parameter 1: Job number
# Parameter 2: Number of jobs
#
# To test if any test is missing:
# for i in $(seq $NUM_JOBS); do
#    ./scripts/filter_bats_list.bash $i $NUM_JOBS >> list;
# done

JOB_NUM=$1
NUM_JOBS=$2
DEFAULT_WEIGHT=6

TEST_LIST=$(find test/functional/ -name "*.bats" | sort)

TOTAL_WEIGHT=0
for t in $TEST_LIST; do
    WEIGHT=$(sed -n -e 's/^#WEIGHT=//p' "$t")
    if [ -z "$WEIGHT" ]; then
        WEIGHT="$DEFAULT_WEIGHT"
    fi
    TOTAL_WEIGHT=$((TOTAL_WEIGHT + WEIGHT))
done

NUM_TESTS="$((TOTAL_WEIGHT/(NUM_JOBS)))"
START="$(((JOB_NUM - 1) * NUM_TESTS + 1))"
END="$((JOB_NUM * NUM_TESTS))"

COUNT=0
for t in $TEST_LIST; do
    WEIGHT=$(sed -n -e 's/^#WEIGHT=//p' "$t")
    if [ -z "$WEIGHT" ]; then
        WEIGHT="$DEFAULT_WEIGHT"
    fi

    COUNT=$((COUNT + WEIGHT))

    if [ "$JOB_NUM" -ne "$NUM_JOBS" ] && [ $COUNT -gt $END ]; then
        break
    fi

    if [ $COUNT -ge $START ]; then
        echo "$t"
    fi

done
