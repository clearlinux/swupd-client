#
# This file is part of swupd-client.
#
# Copyright © 2017-2025 Intel Corporation
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 2 or later of the License.
#
#

# Build with the following:
# docker build -t swupd-ci -f docker/Dockerfile .

FROM clearlinux:latest
RUN swupd bundle-add os-core-update-dev sysadmin-basic os-testsuite c-basic haskell-basic
RUN pip install coverxygen && stack install ShellCheck && mv /root/.local/bin/shellcheck /usr/bin/
RUN useradd tester && usermod -aG wheelnopw tester
COPY . /build
RUN chown -R tester /build
USER tester
WORKDIR /build

# Requires docker run with: --cap-add LINUX_IMMUTABLE
CMD ["/build/docker/dockerrun.sh"]
