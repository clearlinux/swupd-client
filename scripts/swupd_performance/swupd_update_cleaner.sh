#!/usr/bin/bash

if [ -z "$1" ]; then
	set -- "/var/log/swupd/staging/"
fi

SAVEIFS=$IFS
IFS=$'\n'
files="$(grep -l 'System already up-to-date' -r $1)"
IFS=$SAVEIFS

for (( i=0; i<${#files[@]}; i++ ))
do
    rm -r "${files[$i]}" || echo "true"
    echo "Removing file: ${files[$i]} as it is already updated"
done
