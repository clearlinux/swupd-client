#!/bin/bash

TMPDIR=$(mktemp -d)
PKG="swupd-client"
LIB="libswupd"

OLDVERSION=$(git tag -l 'swupd-client*' | sort -t . -k 1,1n -k 2,2n | tail -1)
NEWVERSION="HEAD"

OLDCOMMIT=$(git rev-list -n 1 --abbrev-commit ${OLDVERSION})
NEWCOMMIT=$(git rev-list -n 1 --abbrev-commit ${NEWVERSION})

do_build() {
  local commit=$1
  echo "Building ${PKG} at commit ${commit}..."
  git archive \
    -o "${TMPDIR}/${PKG}-${commit}.tar.gz" \
    --prefix="${PKG}-${commit}/" ${commit}
  (
    pushd "${TMPDIR}"
    tar xf ${PKG}-${commit}.tar.gz
    pushd ${PKG}-${commit}
    autoreconf -fi
    ./configure
    make
    popd
    popd
  ) &> /dev/null
}

create_xml_desc() {
  local commit=$1
  cat > ${TMPDIR}/${PKG}-${commit}.xml << EOF
<version>
${commit}
</version>
<headers>
${TMPDIR}/${PKG}-${commit}/include/swupd.h
</headers>
<libs>
${TMPDIR}/${PKG}-${commit}/.libs
</libs>
EOF
}

bump_current() {
  local current=$(grep '^LIBSWUPD_CURRENT' Makefile.am | cut -d'=' -f2)
  current=$(expr $current + 1)
  sed -i "s/^\(LIBSWUPD_CURRENT=\).*$/\1$current/" Makefile.am
  # Note: the REVISION and AGE fields are not modified at all right now, but
  # when logic is implemented in the future according to the libtool algorithm,
  # they must be reset to zero, as below.
  sed -i "s/^\(LIBSWUPD_REVISION=\).*$/\10/" Makefile.am
  sed -i "s/^\(LIBSWUPD_AGE=\).*$/\10/" Makefile.am
  git add Makefile.am
  git commit -s -m "Bump $LIB major version to $current"
}

do_build ${OLDCOMMIT}
do_build ${NEWCOMMIT}

create_xml_desc ${OLDCOMMIT}
create_xml_desc ${NEWCOMMIT}

abi-compliance-checker \
  -lib ${LIB} \
  -report-path ${TMPDIR}/reports/${OLDCOMMIT}_to_${NEWCOMMIT}/report.xml \
  -log1-path ${TMPDIR}/logs/${OLDCOMMIT}.log \
  -log2-path ${TMPDIR}/logs/${NEWCOMMIT}.log \
  -old ${TMPDIR}/${PKG}-${OLDCOMMIT}.xml \
  -new ${TMPDIR}/${PKG}-${NEWCOMMIT}.xml \
  -xml

ret=$?

if [ $ret -eq 0 ]; then
  # TODO: should bump AGE and CURRENT for backward-compatible library changes,
  # and REVISION for any library code changes.
  echo "No compatibility errors found!"
elif [ $ret -eq 1 ]; then
  echo "Incompatibilities found; bumping lib version."
  bump_current
elif [ $ret -gt 1 ]; then
  echo "Problem running the ABI check tool: error code ${ret}."
fi

exit $ret

# vi: ts=8 sw=2 sts=2 et tw=80
