#!/usr/bin/python3

"""
Module to process output files in JSON from swupd-update service.

This module is created to clean, parse output of swupd update service
and create a CSV file. This module expects files to have a particular filename
in an input folder.

"""

import argparse
import csv
import fileinput
import json
import logging
import os
from datetime import datetime


class ExecutionStatus(object):
    skipped_files = 0
    total_files = 0
    rows_added = 0


logging.basicConfig(level=logging.DEBUG)


JSON_KEYWORD1 = ["changed bundles", "new bundles", "deleted bundles",
                 "changed files", "new files", "deleted files",
                 "Statistics for going from version", "timestamp"]

FILTER1 = ["info"]

STATE_FILE = ".state"

DEFAULT_INPUT_FOLDER = "/var/log/swupd/staging"

DEFAULT_OUT_FILE = "performance_sheet.csv"

# keyword_list3 = ['Total execution time']


def sanitize_get_item(item):
    """Clean and parse item.

    Clean and parse item of type 'info', split and return key, value.

    Args:
        item (dict): item to be cleaned

    Returns:
        a tuple of key, value. returns unknowns strings
        on Keyerror

    """
    key = "unknown"
    value = "unknown"
    try:
        key = item['msg'].split(":")[0].strip()
        value = item['msg'].split(":")[1].strip()
    except KeyError:
        pass
    return key, value


def get_to_from_version(stat_string):
    """Extract to, from version from stat_string.

    Extract the to & from version from a string
    which has statistics string in it.

    Args:
        stat_string (string): string to be cleaned

    Returns:
        a tuple of (from_version, to_version) or 0 is returned for a
        field whose value cant be determined

    """
    integers_parsed = [0, 0]

    if len(stat_string) < 3:
        return integers_parsed

    i = 0
    for item in stat_string.split():
        try:
            integers_parsed[i] = int(item)
            i = i + 1
        except ValueError:
            pass
    return tuple(integers_parsed)


def bundle_stats(bundle_file_stats, outputfile):
    """Parse a list of dictionary and writes to outputfile in csv format.

    Parse a list of dicts called bundle_file_stats
    constructed from the incoming json file and writes it to an outputfile.

    Args:
        bundle_file_stats (list of dict): list of dicts to parse values from
        outputfile (string): the csv file to write to

    """
    fieldnames = ['changed bundles', 'new bundles',
                  'deleted bundles', 'changed files',
                  'new files', 'deleted files',
                  'from_version', 'to_version', 'timestamp']
    writer = None
    bundle_stat_dict = {}
    file_exist = os.path.isfile(outputfile)

    with open(outputfile, "a") as outfile:
        writer = csv.DictWriter(
            outfile,
            fieldnames=fieldnames,
            lineterminator='\n')

        if not file_exist:
            writer.writeheader()

        for keyword in JSON_KEYWORD1:
            for item in bundle_file_stats:
                if item['msg'].find(keyword) > -1:
                    (key, value) = sanitize_get_item(item)
                    if keyword == "Statistics for going from version":
                        (bundle_stat_dict['from_version'],
                         bundle_stat_dict['to_version']) = \
                                 get_to_from_version(key)
                    else:
                        bundle_stat_dict[keyword] = value
        writer.writerow(bundle_stat_dict)


# This function is a WIP dont use it

# def execution_time(file_stats):
#    od = OrderedDict()
#
#    for items in file_stats:
#        for k,v in items.items():
#            od[k] = v
#    print(od)
#    with open("test.csv", "a") as outfile:
#        for keyword in keyword_list3:
#            for item in bundle_file_stats:
#                if item['msg'].find(keyword) > -1:
#                    key = item['msg'].split(':')[0].strip()
#                    value = item['msg'].split(":")[1].strip()
#                    if (keyword in JSON_KEYWORD1):
#                        outfile.write(key + last_item(keyword_list3,
#                            keyword))
#                    else:
#                        outfile.write(value + last_item(keyword_list3,
#                            keyword))
#    pass

def update_already_done(fpath):
    """Check if file says update performed or not.

    Check a file to see if performed an update or not

    Args:
        fpath (string): path of the file to be checked

    Returns:
        bool: True if the file says output has been done, False otherwise

    """
    keyword = "System already up-to-date"
    with open(fpath, "r") as inp:
        try:
            data = json.load(inp)
            filtered_data = [x for x in data if x['type'] in FILTER1]
            for item in filtered_data:
                if item['msg'].find(keyword) > -1:
                    return True
        except json.decoder.JSONDecodeError:
            logging.error("file could be parsed: %s", fpath)
    return False


def make_file_json_parsable(fpath):
    """Check if file is json parsable or not.

    Make sure each file is json parsable
    by checking for jsonic tags.

    Args:
        fpath (string): path of the file to be checked

    Returns:
        bool: True if it is json_parsable or False otherwise

    """
    for line in fileinput.input(fpath, inplace=True):
        if line.startswith('[') or line.startswith(']') \
                or line.startswith('{') or line.startswith('}'):
            print(line.rstrip('\n'))

    # final check to see that solved it
    with open(fpath, "r") as inp:
        try:
            json.load(inp)
        except json.decoder.JSONDecodeError:
            logging.error("File could not be parsed by JSON: %s", fpath)
            return False
    return True


def mark_file_as_done(fname):
    """Mark a file in record file as processed.

    Create a file if it does not
    exist in current working directory
    called a STATE FILE to maintain record of all files processed
    so far to prevent being parsed again.

    Args:
        fname (string): name of the file to marked

    """
    with open(STATE_FILE, "a") as out:
        out.write(fname + "\n")
    logging.info("Marking file as done: %s", fname)


def skip_if_processed(fname):
    """Check if file is processed or not.

    Check if the current file
    needs to be processed or skipped
    by reading if it exists in STATE FILE.

    Args:
        fname (string): name of the file to checked

    Returns:
        bool: True if we can skip this file, False otherwise

    """
    if not os.path.exists(STATE_FILE):
        return False

    with open(STATE_FILE, "r") as inp:
        for line in inp.readlines():
            if fname.rstrip() == line.rstrip():
                return True
    return False


def print_header():
    """Print header for whole operation.

    Prints a pretty header

    """
    print("-----------------------------")
    print("Swupd data processing script")
    print("-----------------------------")


def print_summary():
    """Print summary for whole operation.

    Prints skipped's files, total files, file really processed.

    """
    print("\n")
    print("------------------")
    print("Summary")
    print("------------------")
    print("Skipped files:\t\t", ExecutionStatus.skipped_files)
    print("Processed files:\t", ExecutionStatus.rows_added)
    print("Total files:\t\t", ExecutionStatus.total_files)

    if ExecutionStatus.skipped_files == ExecutionStatus.total_files:
        print("No records were added to an existing csv output file")


def process(inputfolder, outfile):
    """Process each file in input folder.

    Pass each file to `start_processing_each_file`
    if doesnt need to be skipped.

    Args:
        inputfolder (string): name of the folder to processed
        outfile (string): The output csv file to write to

    """
    print_header()

    if not os.path.exists(inputfolder):
        logging.error("input Folder : %s does not exist", inputfolder)
        return

    ExecutionStatus.total_files = len(os.listdir(inputfolder))
    if ExecutionStatus.total_files == 0:
        logging.info("There are no files to be processed")
        return

    for fname in os.listdir(inputfolder):
        print("")
        logging.info("Processing File: %s", fname)
        path = os.path.join(inputfolder, fname)

        if skip_if_processed(fname):
            ExecutionStatus.skipped_files += 1
            logging.info("Skip processing file: %s", fname)
            continue

        # if we fail to make sure file is json parsable, if we fail
        # make it mark it as done
        if not make_file_json_parsable(path):
            mark_file_as_done(fname)
            continue

        # check if the output contains that system is up to update, if yes
        # mark it as done
        if update_already_done(path):
            mark_file_as_done(fname)
            logging.info(
                "file: %s says system is up-to-date", fname)
            continue

        start_processing_each_file(path, outfile)
        ExecutionStatus.rows_added += 1
        mark_file_as_done(fname)

    print_summary()


def parse_timestamp_from_filename(fname):
    """Parse & Extract timestamp from filename.

    Parse each file_name to extract the
    timestamp of its creation for the csv outfile field.

    Args:
        fname (string): name of file

    Returns:
        datatime obj or None if datetime parsing failed

    """
    date_obj = None
    index = fname.find("file_")
    try:
        if index > -1:
            timestamp = fname[index + 5:-5]
            date_obj = datetime.strptime(timestamp, '%Y%m%d_%H%M%S%Z')
    except (IndexError, ValueError):
        pass
    return date_obj


def start_processing_each_file(fname, outfile):
    """Process file one by one.

    Filter the json stream from each file
    and then writes it to outfile. Called by
    process.

    Args:
        fname (string): name of file
        outfile (string): name of output csv file

    """
    outfile = os.path.abspath(outfile)

    with open(fname) as infile:
        data = json.load(infile)

    # round 1 filtering
    result = [x for x in data if x['type'] in FILTER1]

    # round 2 filtering
    result2 = []
    for keyword in JSON_KEYWORD1:
        temp = [x for x in result if x['msg'].find(keyword) > -1]
        result2.extend(temp)

    date_time = parse_timestamp_from_filename(fname)
    if date_time:
        result2.append(
            {'msg': 'timestamp: ' + date_time.strftime('%Y%m%d_%H%M%S')})
    else:
        result2.append({'msg': 'timestamp: ' + "unknown"})
    bundle_stats(result2, outfile)

    # execution_time(result)


def main():
    """Enter through Main function."""
    argparser = argparse.ArgumentParser()

    argparser.add_argument("-f", "--folder", required=False,
                           default=DEFAULT_INPUT_FOLDER,
                           help="Folder to read results from, \
                    [DEFAULT]: %s " % DEFAULT_INPUT_FOLDER)

    argparser.add_argument(
        "-o",
        "--output",
        required=False,
        default=DEFAULT_OUT_FILE,
        help="Abs path for CSV file to be created.. \
                [DEFAULT]: %s in current working directory"
        % DEFAULT_OUT_FILE)

    args = vars(argparser.parse_args())

    process(args['folder'], args['output'])


if __name__ == '__main__':
    main()
