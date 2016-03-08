#!/bin/bash

set -e

targetfile=e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3

tar -C web-dir/10 -cf web-dir/10/Manifest.MoM.tar Manifest.MoM Manifest.MoM.signed
tar -C web-dir/10 -cf web-dir/10/Manifest.os-core.tar Manifest.os-core Manifest.os-core.signed

sudo chown root:root target-dir/usr/lib/kernel/testfile
sudo chown root:root web-dir/10/files/$targetfile
tar -C web-dir/10/files -cf web-dir/10/files/$targetfile.tar $targetfile
