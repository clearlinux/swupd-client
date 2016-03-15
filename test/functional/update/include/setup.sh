#!/bin/bash

set -e

rm -f web-dir/*/*.tar
tar -C web-dir/10  -cf web-dir/10/Manifest.MoM.tar Manifest.MoM Manifest.MoM.signed
tar -C web-dir/10 -cf web-dir/10/Manifest.test-bundle1.tar Manifest.test-bundle1 Manifest.test-bundle1.signed
tar -C web-dir/10 -cf web-dir/10/Manifest.test-bundle2.tar Manifest.test-bundle2 Manifest.test-bundle2.signed
tar -C web-dir/10 -cf web-dir/10/Manifest.os-core.tar Manifest.os-core Manifest.os-core.signed
tar -C web-dir/100  -cf web-dir/100/Manifest.MoM.tar Manifest.MoM Manifest.MoM.signed
tar -C web-dir/100 -cf web-dir/100/Manifest.test-bundle1.tar Manifest.test-bundle1 Manifest.test-bundle1.signed
tar -C web-dir/100 -cf web-dir/100/Manifest.test-bundle2.tar Manifest.test-bundle2 Manifest.test-bundle2.signed
tar -C web-dir/100 -cf web-dir/100/Manifest.test-bundle3.tar Manifest.test-bundle3 Manifest.test-bundle3.signed
tar -C web-dir/100 -cf web-dir/100/Manifest.test-bundle4.tar Manifest.test-bundle4 Manifest.test-bundle4.signed
tar -C web-dir/100 -cf web-dir/100/Manifest.test-bundle5.tar Manifest.test-bundle5 Manifest.test-bundle5.signed
tar -C web-dir/100 -cf web-dir/100/Manifest.test-bundle6.tar Manifest.test-bundle6 Manifest.test-bundle6.signed
tar -C web-dir/100 -cf web-dir/100/Manifest.test-bundle7.tar Manifest.test-bundle7 Manifest.test-bundle7.signed
tar -C web-dir/100 -cf web-dir/100/Manifest.os-core.tar Manifest.os-core Manifest.os-core.signed
sudo chown root:root web-dir/100/files/24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af
sudo chown root:root web-dir/100/files/d56e570097c2932ff0454cb45ef37964c6a0792e0224013f9e1d837cc22aa9fa
sudo chown root:root web-dir/100/files/95a84952523cf67d92b9f4f03057ca1bf6e1ff018104f7f7ad27d5d4fb38b9ad
sudo chown root:root web-dir/100/files/82206f761ee900b5227656f443f9ac4beb32c25573da6b59bf597e894f4eceac
sudo chown root:root web-dir/100/files/6e5658ceb83e69365ded0684df8f38bcc632f71780ae10ff19b87616a03d6337
sudo chown root:root web-dir/100/files/7726ea82e871eb80c9a72ef95e83ae00e2e8097e7d99ce43b8e47afb1113414b
sudo chown root:root web-dir/100/files/34c8113c07c33167de84e9b9c5557895c958aece3519669706a63a0dafee2481
sudo chown root:root web-dir/100/files/39e14cebdd7f0cab5089a9cecf97f5e77b226d00325e22a0a9081eb544e0b6f0
tar -C web-dir/100/files -cf web-dir/100/files/24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af.tar 24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af --exclude=24d8955d9952c3fcb2241b0f8d225205a5861cec9757b3a075d34810da9b08af/*
tar -C web-dir/100/files -cf web-dir/100/files/d56e570097c2932ff0454cb45ef37964c6a0792e0224013f9e1d837cc22aa9fa.tar d56e570097c2932ff0454cb45ef37964c6a0792e0224013f9e1d837cc22aa9fa
tar -C web-dir/100/files -cf web-dir/100/files/95a84952523cf67d92b9f4f03057ca1bf6e1ff018104f7f7ad27d5d4fb38b9ad.tar 95a84952523cf67d92b9f4f03057ca1bf6e1ff018104f7f7ad27d5d4fb38b9ad
tar -C web-dir/100/files -cf web-dir/100/files/82206f761ee900b5227656f443f9ac4beb32c25573da6b59bf597e894f4eceac.tar 82206f761ee900b5227656f443f9ac4beb32c25573da6b59bf597e894f4eceac
tar -C web-dir/100/files -cf web-dir/100/files/6e5658ceb83e69365ded0684df8f38bcc632f71780ae10ff19b87616a03d6337.tar 6e5658ceb83e69365ded0684df8f38bcc632f71780ae10ff19b87616a03d6337
tar -C web-dir/100/files -cf web-dir/100/files/7726ea82e871eb80c9a72ef95e83ae00e2e8097e7d99ce43b8e47afb1113414b.tar 7726ea82e871eb80c9a72ef95e83ae00e2e8097e7d99ce43b8e47afb1113414b
tar -C web-dir/100/files -cf web-dir/100/files/34c8113c07c33167de84e9b9c5557895c958aece3519669706a63a0dafee2481.tar 34c8113c07c33167de84e9b9c5557895c958aece3519669706a63a0dafee2481
tar -C web-dir/100/files -cf web-dir/100/files/39e14cebdd7f0cab5089a9cecf97f5e77b226d00325e22a0a9081eb544e0b6f0.tar 39e14cebdd7f0cab5089a9cecf97f5e77b226d00325e22a0a9081eb544e0b6f0
