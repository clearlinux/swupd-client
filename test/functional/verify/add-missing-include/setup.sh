#!/bin/bash

set -e

rm -f web-dir/*/*.tar
rm -f web-dir/*/files/*.tar

tar -C web-dir/10  -cf web-dir/10/Manifest.MoM.tar Manifest.MoM Manifest.MoM.signed
tar -C web-dir/10 -cf web-dir/10/Manifest.os-core.tar Manifest.os-core Manifest.os-core.signed
tar -C web-dir/10 -cf web-dir/10/Manifest.test-bundle1.tar Manifest.test-bundle1 Manifest.test-bundle1.signed
tar -C web-dir/10 -cf web-dir/10/Manifest.test-bundle2.tar Manifest.test-bundle2 Manifest.test-bundle2.signed

sudo chown root:root web-dir/10/files/491685caf9cf95df0f721254748df4717b4159513a3e0170ff5fa404dccc32e7
tar -C web-dir/10/files -cf web-dir/10/files/491685caf9cf95df0f721254748df4717b4159513a3e0170ff5fa404dccc32e7.tar 491685caf9cf95df0f721254748df4717b4159513a3e0170ff5fa404dccc32e7
sudo chown root:root web-dir/10/files/826e0a73bdb6ca4863842b07ead83a55b478a950fefd2b1832b30046ce1c1550
tar -C web-dir/10/files -cf web-dir/10/files/826e0a73bdb6ca4863842b07ead83a55b478a950fefd2b1832b30046ce1c1550.tar 826e0a73bdb6ca4863842b07ead83a55b478a950fefd2b1832b30046ce1c1550
sudo chown root:root web-dir/10/files/a96e0b959874854750e8e08372e62c4d1821c5e0106694365396d02c363ada50
tar -C web-dir/10/files -cf web-dir/10/files/a96e0b959874854750e8e08372e62c4d1821c5e0106694365396d02c363ada50.tar a96e0b959874854750e8e08372e62c4d1821c5e0106694365396d02c363ada50
