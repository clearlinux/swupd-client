#!/bin/bash

input=/usr/bin/bash
enc="full"

if [ $# -lt 1 ]; then
	echo -e "\tUsage: creatediffs.sh folder-to-diff [<optional-file-to-diff-against>] <optional_encoding>\n"
	exit
fi

if [ $# -ge 2 ]; then
	input=$2
fi

if [ $# -eq 3 ]; then
	enc=$3
fi

folder=$1

# Using find because rm fails when there are > 4096 files
echo "* Cleaning up output folders..."
find diffs/* -name "*" -delete > /dev/null 2>&1
find failed/* -name "*" -delete > /dev/null 2>&1
find fulldownload/* -name "*" -delete > /dev/null 2>&1
find patched/* -name "*" -delete > /dev/null 2>&1
rm -rf errors/errordiffs errors/falsepositivediffs errors/hashfails RESULTS.txt

differr=0
patcherr=0
falsepos=0
hasherr=0

# Run bsdiff with every supported encoding
echo "* Running bsdiff..."
if [ "$enc" == "full" ]; then
	for f in $(ls $folder);
	do
		for t in $(cat types);
		do
			echo "$input ----> $f TYPE: $t"
			sudo bsdiff $input $folder/$f diffs/$f.$t $t #> /dev/null
			ret=$?
			if [ $ret -eq 255 ]; then
				echo -e "***ERROR: $ret\n"
				sudo mv diffs/$f.$t failed/$f.$t
				let differr=differr+1
			elif [ $ret -eq 1 ]; then
				echo -e "\t* FULL DOWNLOAD requested"
				sudo mv diffs/$f.$t fulldownload/$f.$t
			fi
		done
	done
else
	# Do diffs with ONLY the specified encoding if given
	echo "IN MINIMAL"
	for f in $(ls $folder);
	do
		echo "$input ----> $f TYPE: $enc"
		sudo valgrind bsdiff $input $folder/$f diffs/$f.$enc $enc
		ret=$?
		if [ $ret -eq 255 ]; then
			echo -e "***ERROR: $ret\n"
			sudo mv diffs/$f failed/$f
			let differr=differr+1
		elif [ $ret -eq 1 ]; then
			sudo mv diffs/$f fulldownload/$f
		fi
	done
fi

# Check if the successful diffs REALLY do apply cleanly
echo -e "\n* Applying created diffs..."

for f in $(ls diffs);
do
	sudo bspatch $input patched/$f diffs/$f
	ret=$?

	if [ $ret -ne 0 ]; then
		echo -e "Failed to apply diff $f\n"
		let patcherr=patcherr+1
		sudo echo "$f, $ret" >> errors/errordiffs
	fi
done
echo -e "Finished!\n"

# Check if any failed diffs apply cleanly to mark false positives
echo -e "* Applying failed diffs..."
for f in $(ls failed);
do
	sudo bspatch $input patched/$f-FAILED failed/$f
	ret=$?

	if [ $ret -eq 0 ]; then
		echo "FALSEPOSITIVE: $f applied successfully"
		echo "$f, $ret" >> errors/falsepositivediffs
		let falsepos=falsepos+1
	fi
done
echo -e "Finished!\n"

# Check that the patched file hashes match the original file hashes
for f in $(ls patched);
do
	newhash=$(sudo swupd hashdump --basepath ./ patched/$f | tail -1)
	# strip the encoding type off the filename so we can match the original file
	oldfile=$(echo $f | sed 's/\.[a-z0-9]*$//')
	oldhash=$(sudo swupd hashdump --basepath $folder $oldfile | tail -1)

	if [[ "$newhash" != "$oldhash" ]]; then
		echo -e "\n*** ERROR: hash mismatch **\n$input/$oldfile\n"
		echo -e "patched/$f\nHas Hash: $newhash\nExpected: $oldhash\n" >> errors/hashfails
		echo -e "NEWHASH: $newhash\nOLDHASH: $oldhash"
		let hasherr=hasherr+1
	fi
done

# Report the number of failures since a lot of output was probably produced
echo "Failed Diffs: $differr" | tee -a RESULTS.txt
echo "Failed patches: $patcherr" | tee -a RESULTS.txt
echo "False positive diffs: $falsepos" | tee -a RESULTS.txt
echo "Hash Mismatches: $hasherr" | tee -a RESULTS.txt
echo
