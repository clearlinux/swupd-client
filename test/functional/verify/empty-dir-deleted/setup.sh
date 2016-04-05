#!/bin/bash

set -e

tar -C web-dir/10 -cf web-dir/10/Manifest.MoM.tar Manifest.MoM Manifest.MoM.signed
tar -C web-dir/10 -cf web-dir/10/Manifest.os-core.tar Manifest.os-core Manifest.os-core.signed
rm target-dir/testdir/.gitignore
