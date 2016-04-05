#!/bin/bash

set -e

rm -f web-dir/*/*.tar
mkdir -p target-dir/testdir
touch target-dir/testdir/.gitignore
