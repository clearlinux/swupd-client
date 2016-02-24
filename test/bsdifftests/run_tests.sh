#!/bin/bash

mkdir -p ./{diffs,errors,failed,fulldownload,patched,results}

sudo find results/* -name "*" -delete

function run_test()
{
	mkdir results/$1
	./creatediffs.sh $2 $3
	sudo cp -r diffs errors failed fulldownload patched results/$1
	sudo mv RESULTS.txt results/$1
	echo -e "[$test]\n$(cat results/$test/RESULTS.txt)\n" >> results/ALL_RESULTS.txt
}

# To write a test:
# run_test <folder_to_diff_files_from> <file_to_diff_with>
# This will attempt to create a diff from given file -> each file in folder

test="empty_test"
echo -e "[ Running emtpy->full file diff test ]"
echo -e "--------------------------------------\n"
run_test $test inputs/miscellaneous inputs/miscellaneous/empty_mem.c

test="text_tests"
echo -e "[ Running simple text file test ]"
echo -e "---------------------------------\n"
run_test $test inputs/text inputs/text/counter-api.txt 

test="html_tests"
echo -e "[ Running html file diff test ]"
echo -e "-------------------------------\n"
run_test $test inputs/html inputs/html/840_update.html

test="xml_tests"
echo -e "[ Running xml file diff test ]"
echo -e "------------------------------\n"
run_test $test inputs/xml inputs/xml/log.xml

test="xml_html_tests"
echo -e "[ Running html -> xml file diff test ]"
echo -e "--------------------------------------\n"
run_test $test inputs/xml inputs/html/840_update.html

test="html_xml_tests"
echo -e "[ Running xml -> html file diff tests ]"
echo -e "---------------------------------------\n"
run_test $test inputs/html inputs/xml/log.xml

test="empty_to_xml"
echo -e "[ Running empty-> XML test ]"
echo -e "----------------------------\n"
run_test $test inputs/xml inputs/miscellaneous/empty_mem.c

test="empty_to_html"
echo -e "[ Running empty-> HTML test ]"
echo -e "-----------------------------\n"
run_test $test inputs/html inputs/miscellaneous/empty_mem.c
