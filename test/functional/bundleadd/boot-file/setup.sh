#!/bin/bash

set -e

targetfile=e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3

tar -C web-dir/10 -cf web-dir/10/Manifest.MoM.tar Manifest.MoM Manifest.MoM.signed
tar -C web-dir/10 -cf web-dir/10/Manifest.test-bundle.tar Manifest.test-bundle Manifest.test-bundle.signed
tar -C web-dir/10 -cf web-dir/10/pack-test-bundle-from-0.tar staged/$targetfile

sudo chown root:root web-dir/10/staged/$targetfile
