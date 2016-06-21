#!/bin/sh

if [ $# -ne 2 ]; then
  echo "Usage: $0 VER BUNDLE"
  echo "Emits the manifest hash for BUNDLE at this VER"
  exit 1
fi

VER=$1
BUNDLE=$2
MANIFEST="web-dir/$VER/Manifest.$BUNDLE"

if [ $EUID = 0 ]; then
  echo "Run this script as non-root. It will call sudo as needed"
  exit 1
fi

sudo chown root:root $MANIFEST
HASH=$(sudo swupd hashdump -b $(pwd) $MANIFEST | tail -n 1)
sudo chown $EUID:$EUID $MANIFEST

echo $HASH
