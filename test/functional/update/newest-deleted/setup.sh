#!/bin/bash

set -e

rm -f web-dir/*/*.tar

tar -C web-dir/10 -cf web-dir/10/Manifest.MoM.tar Manifest.MoM Manifest.MoM.signed
tar -C web-dir/10 -cf web-dir/10/Manifest.os-core.tar Manifest.os-core Manifest.os-core.signed
tar -C web-dir/10 -cf web-dir/10/Manifest.test-bundle1.tar Manifest.test-bundle1 Manifest.test-bundle1.signed
tar -C web-dir/20 -cf web-dir/20/Manifest.MoM.tar Manifest.MoM Manifest.MoM.signed
tar -C web-dir/20 -cf web-dir/20/Manifest.os-core.tar Manifest.os-core Manifest.os-core.signed
tar -C web-dir/20 -cf web-dir/20/Manifest.test-bundle2.tar Manifest.test-bundle2 Manifest.test-bundle2.signed
tar -C web-dir/30 -cf web-dir/30/Manifest.MoM.tar Manifest.MoM Manifest.MoM.signed
tar -C web-dir/30 -cf web-dir/30/Manifest.os-core.tar Manifest.os-core Manifest.os-core.signed
tar -C web-dir/30 -cf web-dir/30/Manifest.test-bundle2.tar Manifest.test-bundle2 Manifest.test-bundle2.signed

os_release_file=1c1efd10467cbc1dddd8e63c3ef4d9099f36418ed5372d080a1bf0f03a49ab05
sudo chown root:root web-dir/30/staged/$os_release_file
tar -C web-dir/30 -cf web-dir/30/pack-os-core-from-20.tar staged/$os_release_file
cp target-dir/testfile $PWD
