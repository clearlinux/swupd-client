#!/usr/bin/bash

if [ -z "$1" ]; then
	set -- "/var/log/swupd/staging/"
fi

/usr/bin/swupd update --json-output --time > "$1"/file_"$(date +%Y%m%d_%H%M%S%Z)".json
