#!/bin/bash

set -e

rm -f web-dir/*/*.tar
tar -C web-dir/10  -cf web-dir/10/Manifest.MoM.tar Manifest.MoM Manifest.MoM.signed
tar -C web-dir/10 -cf web-dir/10/Manifest.test-bundle.tar Manifest.test-bundle Manifest.test-bundle.signed
tar -C web-dir/10 -cf web-dir/10/Manifest.os-core.tar Manifest.os-core Manifest.os-core.signed
sudo chown root:root web-dir/10/staged/24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af
sudo chown root:root web-dir/10/staged/e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3
tar -C web-dir/10 -cf web-dir/10/pack-test-bundle-from-0.tar staged/24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af --exclude=staged/24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af/*
tar -C web-dir/10 -cf web-dir/10/pack-os-core-from-0.tar staged/e6d85023c5e619eb43d5cfbfdbdec784afef5a82ffa54e8c93bda3e0883360a3
