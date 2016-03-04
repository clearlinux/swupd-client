#!/bin/bash

set -e

rm -f web-dir/*/*.tar
tar -C web-dir/10  -cf web-dir/10/Manifest.MoM.tar Manifest.MoM Manifest.MoM.signed
tar -C web-dir/100  -cf web-dir/100/Manifest.MoM.tar Manifest.MoM Manifest.MoM.signed
tar -C web-dir/10  -cf web-dir/10/Manifest.os-core.tar Manifest.os-core Manifest.os-core.signed
tar -C web-dir/10 -cf web-dir/10/Manifest.test-bundle.tar Manifest.test-bundle Manifest.test-bundle.signed
tar -C web-dir/100 -cf web-dir/100/Manifest.test-bundle.tar Manifest.test-bundle Manifest.test-bundle.signed
sudo chown root:root target-dir/usr
sudo chmod 755 target-dir/usr
sudo chown root:root web-dir/10/files/24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af
sudo chown root:root web-dir/100/files/c2fb5e545287e632581b82e371de5d4aac2c7eb50d7f357f3b66fb0fdca71dd9
tar -C web-dir/100/files -cf web-dir/100/files/c2fb5e545287e632581b82e371de5d4aac2c7eb50d7f357f3b66fb0fdca71dd9.tar c2fb5e545287e632581b82e371de5d4aac2c7eb50d7f357f3b66fb0fdca71dd9
tar -C web-dir/10/files -cf web-dir/10/files/24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af.tar 24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af --exclude=24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af/*
