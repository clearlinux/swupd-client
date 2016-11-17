#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.66])
AC_INIT(swupd-client, 3.7.2, tudor.marcu@intel.com)
AM_PROG_AR
LT_INIT
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -W subdir-objects])
AM_SILENT_RULES([yes])
AC_PROG_CC
AM_PROG_CC_C_O
AC_LANG(C)

AC_CONFIG_HEADERS([config.h])
PKG_CHECK_MODULES([bsdiff], [bsdiff])
PKG_CHECK_MODULES([lzma], [liblzma])
PKG_CHECK_MODULES([zlib], [zlib])
AC_ARG_ENABLE(
	[bzip2],
	AS_HELP_STRING([--disable-bzip2],[Do not use bzip2 compression (uses bzip2 by default)]),
)

BZIP="yes"
AS_IF([test -n "$enable_bzip2" -a "$enable_bzip2" = "yes"],
		[BZIP="$enable_bzip2"]
)

AS_IF([test "x$enable_bzip2" != "xno" ],
  [AC_DEFINE(SWUPD_WITH_BZIP2,1,[Use bzip2 compression])
	 AC_CHECK_LIB([bz2], [BZ2_bzBuffToBuffCompress], [], [AC_MSG_ERROR([the libbz2 library is missing])])
	 BZIP="yes"],
  [AC_DEFINE(SWUPD_WITHOUT_BZIP2,1,[Do not use bzip2 compression])
	 BZIP="no"]
)
AC_ARG_ENABLE(
  [signature-verification],
  [AS_HELP_STRING([--enable-signature-verification], [Enable signature check (disabled by default)])],
  [AC_DEFINE([SIGNATURES], 1, [Enable signature check as default])]
)

SIGVERIFICATION="no"
AS_IF([test -n "$enable_signature_verification" -a "$enable_signature_verification" = "yes" ],
		[SIGVERIFICATION="$enable_signature_verification"]
)

if test "$enable_signature_verification" = "yes" ; then
	AC_ARG_WITH([swupdcert],
		[AS_HELP_STRING([--with-swupdcert=FILE], [swupd verification cert])],
		[AC_DEFINE_UNQUOTED([SWUPDCERT], ["$withval"], [swupd verification cert])],
		[AC_DEFINE([SWUPDCERT], ["ClearLinuxRoot.pem"], [swupd verification cert])])
fi

AS_IF([test -n "$with_swupdcert" -a "$with_swupdcert" != "no" -a "$with_swupdcert" != "yes"],
		[SWUPDCERT="$with_swupdcert"],
	[SWUPDCERT="ClearLinuxRoot.pem"]
)

AC_ARG_WITH([contenturl],
	[AS_HELP_STRING([--with-contenturl=URL], [Default content url])],
	  [AC_DEFINE_UNQUOTED([CONTENTURL], ["$withval"], [Default content url])]
)

AS_IF([test -n "$with_contenturl" -a "$with_contenturl" != "no" -a "$with_contenturl" != "yes"],
		[CONTENTURL="$with_contenturl"],
	[test "$with_contenturl" = "no"],	[CONTENTURL='!! Warning !! --with-contenturl not specified!'],
	[test "$with_contenturl" = "yes"],	[CONTENTURL='!! Warning !! --with-contenturl not specified!'],
	[CONTENTURL='!! Warning !! --with-contenturl not specified!']
)

AC_ARG_WITH([versionurl],
	[AS_HELP_STRING([--with-versionurl=URL], [Default version url])],
	[AC_DEFINE_UNQUOTED([VERSIONURL], ["$withval"], [Default version url])]
)

AS_IF([test -n "$with_versionurl" -a "$with_versionurl" != "no" -a "$with_versionurl" != "yes"],
		[VERSIONURL="$with_versionurl"],
	[test "$with_versionurl" = "no"],	[VERSIONURL='!! Warning !! --with-versionurl not specified!'],
	[test "$with_versionurl" = "yes"],	[VERSIONURL='!! Warning !! --with-versionurl not specified!'],
	[VERSIONURL='!! Warning !! --with-versionurl not specified!']
)

AC_ARG_WITH([formatid],
	[AS_HELP_STRING([--with-formatid=NUM], [Default format identifier])],
	[AC_DEFINE_UNQUOTED([FORMATID], ["$withval"], [Default format identifier])]
)

AS_IF([test -n "$with_formatid" -a "$with_formatid" != "no" -a "$with_formatid" != "yes"],
		[FORMATID="$with_formatid"],
	[test "$with_formatid" = "no"],	[FORMATID='!! Warning !! --with-formatid not specified!'],
	[test "$with_formatid" = "yes"],	[FORMATID='!! Warning !! --with-formatid not specified!'],
	[FORMATID='!! Warning !! --with-formatid not specified!']
)

AC_ARG_ENABLE(
  [tests],
  [AS_HELP_STRING([--disable-tests], [Do not enable unit or functional test framework (enabled by default)])],
)

TESTS="yes"
AS_IF([test -n "$enable_tests" -a "$enable_tests" = "yes" ],
		[TESTS="$enable_tests"]
)

have_coverage=no
AC_ARG_ENABLE(coverage, AS_HELP_STRING([--enable-coverage], [enable test coverage]))
if test "$enable_coverage" = "yes" ; then
        AC_CHECK_PROG(lcov_found, [lcov], [yes], [no])
        if test "$lcov_found" = "no" ; then
                AC_MSG_ERROR([*** lcov support requested but the program was not found])
        else
                lcov_version_major="`lcov --version | cut -d ' ' -f 4 | cut -d '.' -f 1`"
                lcov_version_minor="`lcov --version | cut -d ' ' -f 4 | cut -d '.' -f 2`"
                if test "$lcov_version_major" -eq 1 -a "$lcov_version_minor" -lt 10; then
                        AC_MSG_ERROR([*** lcov version is too old. 1.10 required])
                else
			have_coverage=yes
			AC_DEFINE([COVERAGE], [1], [Coverage enabled])
                fi
        fi
fi
AM_CONDITIONAL([COVERAGE], [test "$have_coverage" = "yes"])

AC_ARG_ENABLE(
	[bsdtar],
	AS_HELP_STRING([--enable-bsdtar], [Use alternative bsdtar command (uses tar by default)])
)
AS_IF([test "x$enable_bsdtar" = "xyes" ],
	[AC_DEFINE(SWUPD_WITH_BSDTAR, 1, [Use bsdtar])]
)
dnl Enable extended attribute support
XATTR="yes"
AC_ARG_ENABLE(
	[xattr],
	AS_HELP_STRING([--enable-xattr],[Use extended file attributes (unused by default)])
)
AS_IF([test "x$enable_xattr" = "xyes"],
	    [AC_DEFINE(SWUPD_WITH_XATTRS,1,[Use extended file attributes])],
	    [XATTR=no]
)
TARSELINUX="yes"
AC_ARG_ENABLE([tar-selinux],
	AS_HELP_STRING([--enable-tar-selinux],[give --selinux option to tar])
)
AS_IF([test "x$enable_tar_selinux" = "xyes"],
	    [AC_DEFINE(SWUPD_TAR_SELINUX,1,[give --selinux option to tar])
	    AS_IF(test "x$XATTR" = "xno",
	    echo "Must have --enable-xattr to have --enable-tar-selinux" >&2
	    AS_EXIT(1))],
	    [TARSELINUX=no]
)

AC_ARG_WITH([systemdsystemunitdir], AS_HELP_STRING([--with-systemdsystemunitdir=DIR],
            [path to systemd system service dir @<:@default=/usr/lib/systemd/system@:>@]), [unitpath=${withval}],
            [unitpath="$($PKG_CONFIG --variable=systemdsystemunitdir systemd)"])
test -z "${unitpath}" && unitpath=/usr/lib/systemd/system
AC_SUBST(SYSTEMD_UNITDIR, [${unitpath}])

AS_IF([test "$enable_tests" != "no"], [
  PKG_CHECK_MODULES([check], [check >= 0.9.12])
  AC_PATH_PROG([have_python3], [python3])
  AS_IF([test -z "${have_python3}"], [
    AC_MSG_ERROR([Must have Python 3 installed to run functional tests])
  ])
  AC_PATH_PROG([have_bats], [bats])
  AS_IF([test -z "${have_bats}"], [
    AC_MSG_ERROR([Must have the Bash Automated Testing System (bats) installed to run functional tests])
  ])
  TESTS="yes"],
  TESTS="no"
)
AM_CONDITIONAL([ENABLE_TESTS], [test "$enable_tests" != "no"])

PKG_CHECK_MODULES([curl], [libcurl])
PKG_CHECK_MODULES([openssl], [libcrypto >= 1.0.2])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_PROGS(TAR, tar)

# default to Linux rootfs build
enable_linux_rootfs_build="yes"
certs_path="/usr/share/clear/update-ca"

# document all options for build variants
## (1) build variants
AH_TEMPLATE([SWUPD_LINUX_ROOTFS],[Enable Linux rootfs build variant])
## (2) variant features
AH_TEMPLATE([SWUPD_WITH_BINDMNTS],[cope with bind mounts over rootfs])
## (3) variant extra options
AH_TEMPLATE([MOUNT_POINT],[The mount point])
AH_TEMPLATE([STATE_DIR],[The state directory for swupd content])
AH_TEMPLATE([LOG_DIR],[Directory for swupd log files])
AH_TEMPLATE([LOCK_DIR],[Directory for lock file])
AH_TEMPLATE([BUNDLES_DIR],[Directory to use for bundles])
AH_TEMPLATE([UPDATE_CA_CERTS_PATH],[Location of CA certificates])
AH_TEMPLATE([MOTD_FILE],[motd file path])

if test "$enable_linux_rootfs_build" = "yes"; then
	AC_DEFINE([SWUPD_LINUX_ROOTFS],1)
	AC_DEFINE([MOUNT_POINT],["/"])
	AC_DEFINE([STATE_DIR],["/var/lib/swupd"])
	AC_DEFINE([LOG_DIR],["/var/log/swupd"])
	AC_DEFINE([LOCK_DIR],["/run/lock"])
	AC_DEFINE([BUNDLES_DIR],["/usr/share/clear/bundles"])
	AC_DEFINE_UNQUOTED([UPDATE_CA_CERTS_PATH],["$certs_path"])
	AC_DEFINE([MOTD_FILE],["/usr/lib/motd.d/001-new-release"])
else
	AC_MSG_ERROR([Unknown build variant])
fi

AC_SUBST([update_ca_certs_path], ["$certs_path"])

AC_CONFIG_FILES([Makefile data/check-update.service data/swupd-update.service])
AC_REQUIRE_AUX_FILE([tap-driver.sh])
AC_OUTPUT

AC_MSG_NOTICE([

------------
swupd-client
------------

Configuration to build swupd-client:

  Content URL:				${CONTENTURL}
  Version URL:				${VERSIONURL}
  Format Identifier:			${FORMATID}
  Signature verification:		${SIGVERIFICATION}
  SSL Certificate file:			${SWUPDCERT}
  Use bzip compression:			${BZIP}
  Run Tests:				${TESTS}
  Use extended file attributes		${XATTR}
  Use --selinux option for tar		${TARSELINUX}
])
