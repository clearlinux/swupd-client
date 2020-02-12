#!/bin/bash
# Filter a list of jobs
# Parameter 1: Job number
# Parameter 2: Number of jobs
#
# To test if any test is missing:
# for i in $(seq $NUM_JOBS); do
#    ./scripts/filter_bats_list2.sh $i $NUM_JOBS >> list;
# done

JOB_NUM=$(($1 - 1))
NUM_JOBS=$2

penalty() {
    i=$1
    TESTS=$2

    if [[ $i == *'slow'* || $i == *'3rd-party'* || $i == *'certificate'* || $i == *'repair'* ]]; then
        TESTS=$((TESTS * 3))
    fi

    echo $TESTS
}

TOTAL_TESTS=0
for i in $(find test/functional/ -name *.bats); do
    TESTS=$(grep "^@test" $i | wc -l)
    TOTAL_TESTS=$((TOTAL_TESTS + TESTS))
done

NUM_TESTS="$((TOTAL_TESTS/(NUM_JOBS - 1)))"
START="$((JOB_NUM * NUM_TESTS + 1))"
END="$(((JOB_NUM + 1) * NUM_TESTS))"

COUNT=0
for i in $(find test/functional/ -name *.bats); do
    TESTS=$(grep "^@test" $i | wc -l)
    TESTS=$(penalty $i $TESTS)

    COUNT=$((COUNT + TESTS))

    if [ $COUNT -gt $END ]; then
        break
    fi

    if [ $COUNT -ge $START ]; then
        echo -n "$i "
    fi

done
