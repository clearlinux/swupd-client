#!/bin/bash

set -e

rm -f web-dir/*/*.tar
tar -C web-dir/10  -cf web-dir/10/Manifest.MoM.tar Manifest.MoM Manifest.MoM.signed
tar -C web-dir/10 -cf web-dir/10/Manifest.os-core.tar Manifest.os-core Manifest.os-core.signed
tar -C web-dir/100  -cf web-dir/100/Manifest.MoM.tar Manifest.MoM Manifest.MoM.signed
tar -C web-dir/100 -cf web-dir/100/Manifest.os-core.tar Manifest.os-core Manifest.os-core.signed
sudo chown root:root web-dir/100/staged/24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af
tar -C web-dir/100 -cf web-dir/100/pack-os-core-from-10.tar staged/24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af --exclude=staged/24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af/*
