#!/bin/bash

set -e

rm -f web-dir/*/*.tar
tar -C web-dir/10 -cf web-dir/10/Manifest.MoM.tar Manifest.MoM Manifest.MoM.signed
tar -C web-dir/10 -cf web-dir/10/Manifest.os-core.tar Manifest.os-core Manifest.os-core.signed
tar -C web-dir/100 -cf web-dir/100/Manifest.MoM.tar Manifest.MoM Manifest.MoM.signed
tar -C web-dir/100 -cf web-dir/100/Manifest.os-core.tar Manifest.os-core Manifest.os-core.signed

deltafile="10-100-9f83d713da9df6cabd2abc9d9061f9b611a207e1e0dd22ed7a071ddb1cc1537a"

# delta files are currently only applied if they live in delta packs
sudo chown root:root web-dir/100/delta/$deltafile
mkdir -p web-dir/100/staged
tar -C web-dir/100 -cf web-dir/100/pack-os-core-from-10.tar delta/$deltafile staged/

# delta was created to apply to a file owned by root
sudo chown root:root target-dir/testfile
