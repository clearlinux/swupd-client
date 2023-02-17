#
# This file is part of swupd-client.
#
# Copyright © 2017 Intel Corporation
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 2 or later of the License.
#
#

FROM clearlinux:latest
RUN swupd bundle-add os-core-update-dev sysadmin-basic os-testsuite c-basic haskell-basic
RUN pip install coverxygen && stack install ShellCheck && mv /root/.local/bin/shellcheck /usr/bin/
RUN useradd tester && usermod -aG wheelnopw tester
USER tester
COPY . /build
WORKDIR /build

# Requires docker run with: --cap-add LINUX_IMMUTABLE
CMD ["/build/docker/dockerrun.sh"]
