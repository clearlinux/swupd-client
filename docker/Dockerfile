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
ADD dockerrun.sh /dockerrun.sh
ADD https://download.clearlinux.org/releases/13010/clear/Swupd_Root.pem /usr/share/clear/update-ca/Swupd_Root.pem
RUN swupd update && swupd bundle-add os-core-update-dev sysadmin-basic os-testsuite c-basic

CMD ["/dockerrun.sh"]
