#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.66])
AC_INIT(swupd-client, 3.0.0, timothy.c.pepper@linux.intel.com)
AM_PROG_AR
LT_INIT
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -W subdir-objects])
AM_SILENT_RULES([yes])
AC_PROG_CC
AM_PROG_CC_C_O
AC_LANG(C)
AC_ARG_WITH([swupdcertsdir],
  [AS_HELP_STRING([--with-swupdcertsdir=DIR], [swupdcertsdir files])],
  [swupdcertsdir=$withval],
  [swupdcertsdir="/usr/share/clear/update-ca"])
AC_SUBST([swupdcertsdir], [$swupdcertsdir])
AC_CONFIG_HEADERS([config.h])
PKG_CHECK_MODULES([bsdiff], [bsdiff])
PKG_CHECK_MODULES([lzma], [liblzma])
PKG_CHECK_MODULES([zlib], [zlib])
AC_ARG_ENABLE(
	bzip2,
	AS_HELP_STRING([--disable-bzip2],[Do not use bzip2 compression (uses bzip2 by default)]),
	AC_DEFINE(SWUPD_WITHOUT_BZIP2,1,[Do not use bzip2 compression]) ,
	AC_DEFINE(SWUPD_WITH_BZIP2,1,[Use bzip2 compression])
	AC_CHECK_LIB([bz2], [BZ2_bzBuffToBuffCompress], [], [AC_MSG_ERROR([the libbz2 library is missing])])
)

AC_ARG_ENABLE(
  [tests],
  [AS_HELP_STRING([--disable-tests], [Do not enable unit or functional test framework (enabled by default)])]
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
])
AM_CONDITIONAL([ENABLE_TESTS], [test "$enable_tests" != "no"])

PKG_CHECK_MODULES([curl], [libcurl])
PKG_CHECK_MODULES([openssl], [libcrypto >= 0.9.8])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_PROGS(TAR, tar)

# default to Linux rootfs build
enable_linux_rootfs_build="yes"

# document all options for build variants
## (1) build variants
AH_TEMPLATE([SWUPD_LINUX_ROOTFS],[Enable Linux rootfs build variant])
## (2) variant features
AH_TEMPLATE([SWUPD_WITH_BINDMNTS],[cope with bind mounts over rootfs])
AH_TEMPLATE([SWUPD_WITH_SELINUX],[handle selinux attributes])
## (3) variant extra options
AH_TEMPLATE([MOUNT_POINT],[The mount point])
AH_TEMPLATE([STATE_DIR],[The state directory for swupd content])
AH_TEMPLATE([LOG_DIR],[Directory for swupd log files])
AH_TEMPLATE([LOCK_DIR],[Directory for lock file])
AH_TEMPLATE([BUNDLES_DIR],[Directory to use for bundles])
AH_TEMPLATE([STAGING_SUBVOL],[Subvolume for staging content])
AH_TEMPLATE([UPDATE_CA_CERTS_PATH],[Location of CA certificates])
AH_TEMPLATE([SIGNATURE_CA_CERT],[CA certificate to use])
AH_TEMPLATE([MOTD_FILE],[motd file path])

if test "$enable_linux_rootfs_build" = "yes"; then
	AC_DEFINE([SWUPD_LINUX_ROOTFS],1)
	AC_DEFINE([MOUNT_POINT],["/"])
	AC_DEFINE([STATE_DIR],["/var/lib/swupd"])
	AC_DEFINE([LOG_DIR],["/var/log/swupd"])
	AC_DEFINE([LOCK_DIR],["/run/lock"])
	AC_DEFINE([BUNDLES_DIR],["/usr/share/clear/bundles"])
	AC_DEFINE([STAGING_SUBVOL],["/"])
	AC_DEFINE([UPDATE_CA_CERTS_PATH],["/usr/share/clear/update-ca"])
	AC_DEFINE([SIGNATURE_CA_CERT],["test-do-not-ship-R0-0.pem"])
	AC_DEFINE([MOTD_FILE],["/usr/lib/motd.d/001-new-release"])
else
	AC_MSG_ERROR([Unknown build variant])
fi

AC_CONFIG_FILES([Makefile data/check-update.service data/check-update.timer])
AC_REQUIRE_AUX_FILE([tap-driver.sh])
AC_OUTPUT
