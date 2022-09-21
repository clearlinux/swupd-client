#!/bin/bash
# Calls build_ci to install dependencies and build swupd.
# Install dependencies needed to run compliance check, documentation check and
# bash shell check

set -e
./scripts/github_actions/build_ci.bash

# Install dependencies from ubuntu
sudo apt-get install shellcheck
sudo apt-get install doxygen
sudo apt-get install clang-format
sudo pip install coverxygen
