#!/bin/bash

set -e

touch target-dir/usr/share/clear/bundles/test-bundle
tar -C web-dir/10  -cf web-dir/10/Manifest.MoM.tar Manifest.MoM Manifest.MoM.signed
tar -C web-dir/10 -cf web-dir/10/Manifest.os-core.tar Manifest.os-core Manifest.os-core.signed
tar -C web-dir/10 -cf web-dir/10/Manifest.test-bundle.tar Manifest.test-bundle Manifest.test-bundle.signed
