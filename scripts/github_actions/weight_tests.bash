#!/bin/bash
MAX_WEIGHT=40

if [ -z "$1" ]; then
    TESTS=$(find test/functional/ -name "*.bats" \( ! -path "*only_in_ci*" \))
else
    TESTS="$*"
fi

for t in $TESTS; do
    echo "Running $t"
    WEIGHT=$(($(/usr/bin/time -f %e "$t" 2>&1 |tail -n 1 |cut -d '.' -f 1)+1))
    echo "$WEIGHT"
    if [ $WEIGHT -gt "$MAX_WEIGHT" ]; then
        WEIGHT="$MAX_WEIGHT"
    fi

    sed -i "/#WEIGHT/d" "$t"
    echo "#WEIGHT=$WEIGHT" >> "$t"
done
