#!/bin/bash

set -e

targetfile=e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3

sudo rm -f target-dir/usr/lib/kernel/testfile
cp web-dir/10/files/$targetfile target-dir/usr/lib/kernel/testfile

rm -f web-dir/*/*.tar
rm -f web-dir/*/files/*.tar
