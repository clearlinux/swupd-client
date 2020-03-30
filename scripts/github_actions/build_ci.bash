#!/bin/bash
# Install all dependencies available on ubuntu-latest from github actions to
# build swupd and run all tests.
# Builds swupd with configuration needed to run swupd tests

set -e
CORES=2

# Install dependencies from ubuntu
sudo apt-get install python3-docutils
sudo apt-get install check
sudo apt-get install bats
sudo ln -s /bin/systemctl /usr/bin/systemctl

# Use rst2man from python3
sudo ln -s /usr/share/docutils/scripts/python3/rst2man /usr/bin/rst2man.py

# Build Swupd
autoreconf --verbose --warnings=none --install --force
./configure CFLAGS="$CFLAGS -fsanitize=address -Werror" --prefix=/usr --with-fallback-capaths="$PWD"/swupd_test_certificates --with-systemdsystemunitdir=/usr/lib/systemd/system --with-config-file-path="$PWD"/testconfig
make -j$CORES

# Needed to initialize the host for auto-update. Without these steps auto-update
# within a --path would just not work
sudo mkdir -p /usr/lib/systemd/system/
sudo cp data/swupd-update.service /usr/lib/systemd/system/
sudo cp data/swupd-update.timer /usr/lib/systemd/system/
sudo systemctl enable swupd-update.timer
sudo systemctl restart swupd-update.timer
