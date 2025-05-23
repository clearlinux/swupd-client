#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.66])
AC_INIT(swupd-client, 7.0.0, william.douglas@intel.com)
AM_PROG_AR
LT_INIT
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -W subdir-objects])
AM_SILENT_RULES([yes])
AC_PROG_CC
AM_PROG_CC_C_O
AC_LANG(C)

AC_CONFIG_HEADERS([config.h])


# Library checks
PKG_CHECK_MODULES([bsdiff], [bsdiff])
PKG_CHECK_MODULES([lzma], [liblzma])
PKG_CHECK_MODULES([zlib], [zlib])
PKG_CHECK_MODULES([curl], [libcurl])
PKG_CHECK_MODULES([openssl], [libcrypto >= 1.0.1])
PKG_CHECK_MODULES([libarchive], [libarchive])
AC_CHECK_LIB([pthread], [pthread_create])


# Program checks
AC_CHECK_PROGS(RST2MAN, rst2man rst2man.py)
AC_CHECK_PROGS(TAR, tar)
AC_DEFINE(_GNU_SOURCE, 1, [Use non standard gnu functions])

# Enable/Disable debug mode
AC_ARG_ENABLE(
  [debug],
  [AS_HELP_STRING([--enable-debug], [Enable debug mode (disabled by default)])]
)
AS_IF(
  [test "x$enable_debug" == "xyes"],
  [AC_DEFINE(DEBUG_MODE, 1, [Debug enabled])]
)

# Enable/Disable force tartar installation
AC_ARG_ENABLE(
  [tartar],
  [AS_HELP_STRING([--enable-force-tartar], [Force all files to be installed using tartar (disabled by default)])]
)
AS_IF(
  [test "x$enable_force_tartar" == "xyes"],
  [AC_DEFINE(FORCE_TARTAR, 1, [Force tartar enabled])]
)


# Enable/disable options
AC_ARG_ENABLE(
  [optimizations],
  [AS_HELP_STRING([--disable-optimizations], [Disable compiler optimizations (enabled by default)])],
  compiler_optimizations="$enableval",
  [AC_SUBST(OPTIMIZE, [2])]
)
AS_IF(
  [test -n "$compiler_optimizations" -a x"$compiler_optimizations" = "xno"],
  [AC_SUBST(OPTIMIZE, [0])],
  [AC_SUBST(OPTIMIZE, [2])]
)
AM_CONDITIONAL([ENABLE_FORTIFY], [test "$enable_optimizations" != "no"])

AC_ARG_ENABLE(
  [bzip2],
  [AS_HELP_STRING([--disable-bzip2], [Do not use bzip2 compression (uses bzip2 by default)])]
)
BZIP="yes"
AS_IF(
  [test -n "$enable_bzip2" -a "$enable_bzip2" = "yes"],
  [BZIP="$enable_bzip2"]
)
AS_IF(
  [test "x$enable_bzip2" != "xno"],
  [AC_DEFINE(SWUPD_WITH_BZIP2, 1, [Use bzip2 compression])
     AC_CHECK_LIB([bz2], [BZ2_bzBuffToBuffCompress], [], [AC_MSG_ERROR([the libbz2 library is missing])])
     BZIP="yes"],
  [AC_DEFINE(SWUPD_WITHOUT_BZIP2, 1, [Do not use bzip2 compression])
     BZIP="no"]
)

AC_ARG_ENABLE(
  [signature-verification],
  [AS_HELP_STRING([--disable-signature-verification], [Disable signature check (enabled by default)])]
)
SIGVERIFICATION="yes"
AS_IF(
  [test "x$enable_signature_verification" != "xno"],
  [AC_DEFINE(SIGNATURES, 1, [Enable signature check])
     SIGVERIFICATION="yes"],
  [SIGVERIFICATION="no"]
)

AC_ARG_ENABLE(
  [third-party],
  [AS_HELP_STRING([--disable-third-party], [Disable 3rd-party (enabled by default)])]
)
THIRDPARTY="yes"
AS_IF(
  [test "x$enable_third_party" != "xno"],
  [AC_DEFINE(THIRDPARTY, 1, [Enable third party])
     THIRDPARTY="yes"],
  [THIRDPARTY="no"]
)
AM_CONDITIONAL([ENABLE_THIRD_PARTY_TESTS], [test "x$enable_third_party" = "xyes"])

AH_TEMPLATE([THIRDPARTY_REPO_PREFIX], [prefix path for third party repos])

AS_IF(
  [test "$enable_third_party" = "yes"],
  [AC_DEFINE([THIRDPARTY_REPO_PREFIX], ["/opt/3rd_party/"])]
)

AC_ARG_ENABLE(
  [tests],
  [AS_HELP_STRING([--disable-tests], [Do not enable unit or functional test framework (enabled by default)])]
)
TESTS="yes"
AS_IF(
  [test -n "$enable_tests" -a "$enable_tests" = "yes"],
  [TESTS="$enable_tests"]
)
AS_IF(
  [test "$enable_tests" != "no"],
  [PKG_CHECK_MODULES([check], [check >= 0.9.12])
     AC_PATH_PROG([have_python3], [python3])
     AS_IF(
       [test -z "${have_python3}"],
       [AC_MSG_ERROR([Must have Python 3 installed to run functional tests])]
     )
     AC_PATH_PROG([have_bats], [bats])
     AS_IF(
       [test -z "${have_bats}"],
       [AC_MSG_ERROR([Must have the Bash Automated Testing System (bats) installed to run functional tests])]
     )
     TESTS="yes"],
  [TESTS="no"]
)
AM_CONDITIONAL([ENABLE_TESTS], [test "$enable_tests" != "no"])

AC_ARG_ENABLE(
  [manpage],
  [AS_HELP_STRING([--disable-manpage], [Disable manpage creation])]
)
AM_CONDITIONAL([ENABLE_MANPAGE], [test "$enable_manpage" != "no"])

have_coverage=no
AC_ARG_ENABLE(
  [coverage],
  [AS_HELP_STRING([--enable-coverage], [enable test coverage])]
)
AS_IF(
  [test "$enable_coverage" = "yes"],
  [AC_CHECK_PROG(lcov_found, [lcov], [yes], [no])
     AS_IF(
       [test "$lcov_found" = "no"],
       [AC_MSG_ERROR([*** lcov support requested but the program was not found])],
       [lcov_version_major="`lcov --version | cut -d ' ' -f 4 | cut -d '.' -f 1`"
          lcov_version_minor="`lcov --version | cut -d ' ' -f 4 | cut -d '.' -f 2`"
          AS_IF(
            [test "$lcov_version_major" -eq 1 -a "$lcov_version_minor" -lt 10],
            [AC_MSG_ERROR([*** lcov version is too old. 1.10 required])],
            [have_coverage=yes
               AC_DEFINE([COVERAGE], [1], [Coverage enabled])]
          )]
     )]
)
AM_CONDITIONAL([COVERAGE], [test "$have_coverage" = "yes"])

AC_ARG_ENABLE(
  [bsdtar],
  [AS_HELP_STRING([--enable-bsdtar], [Use alternative bsdtar command (uses tar by default)])]
)
AS_IF(
  [test "x$enable_bsdtar" = "xyes"],
  [AC_DEFINE(SWUPD_WITH_BSDTAR, 1, [Use bsdtar])]
)

dnl Enable extended attribute support
XATTR="yes"
AC_ARG_ENABLE(
  [xattr],
  [AS_HELP_STRING([--enable-xattr], [Use extended file attributes (unused by default)])]
)
AS_IF(
  [test "x$enable_xattr" = "xyes"],
  [AC_DEFINE(SWUPD_WITH_XATTRS, 1, [Use extended file attributes])],
  [XATTR=no]
)

TARSELINUX="yes"
AC_ARG_ENABLE(
  [tar-selinux],
  [AS_HELP_STRING([--enable-tar-selinux], [give --selinux option to tar])]
)
AS_IF(
  [test "x$enable_tar_selinux" = "xyes"],
  [AC_DEFINE(SWUPD_TAR_SELINUX, 1, [give --selinux option to tar])
    AS_IF(
      [test "x$XATTR" = "xno"],
      [echo "Must have --enable-xattr to have --enable-tar-selinux" >&2
         AS_EXIT(1)]
    )],
  [TARSELINUX=no]
)

AC_ARG_ENABLE(
  [stateless],
  [AS_HELP_STRING([--disable-stateless], [OS is not stateless, do not ignore configuration files (stateless by default)])]
)
AS_IF(
  [test "x$enable_stateless" = "xno"],
  [AC_DEFINE(OS_IS_STATELESS, 0, [OS is not stateless])],
  [AC_DEFINE(OS_IS_STATELESS, 1, [OS is stateless])]
)

external_modules="yes"
AC_ARG_ENABLE(
  [external-modules],
  [AS_HELP_STRING([--disable-external-modules], [Disable external modules (enabled by default)])],
  external_modules="$enableval"
)
AS_IF(
  [test -n "$external_modules" -a x"$external_modules" = "xyes"],
  [AC_DEFINE(EXTERNAL_MODULES_SUPPORT, 1, [external modules are supported])]
)


# With/without options
AC_ARG_WITH(
  [certpath],
  [AS_HELP_STRING([--with-certpath=PATH], [Location of update certificate])]
)
default_cert_path="/usr/share/clear/update-ca/Swupd_Root.pem"
cert_path=
# Makes sure --with-certpath receives an argument that is not "yes" or "no",
# and uses the default path if only --enable-signature-verification is passed.
AS_IF(
  [test "x$enable_signature_verification" != "xno" && test "$with_certpath" = "no"],
  [AC_MSG_ERROR(['--with-certpath=no' or '--without-certpath' not supported. Specify a PATH.])],
  [test "x$enable_signature_verification" != "xno" && test "$with_certpath" = "yes"],
  [AC_MSG_ERROR(['--with-certpath=yes' or '--with-certpath' not supported. Specify a PATH.])],
  [test "x$enable_signature_verification" = "xno" && test -n "$with_certpath"],
  [AC_MSG_WARN([--with-certpath=PATH requires --enable-signature-verification])],
  [test "x$enable_signature_verification" != "xno" && test -n "$with_certpath"],
  [cert_path="$with_certpath"],
  [test "x$enable_signature_verification" != "xno"],
  [cert_path="$default_cert_path"]
)

AC_ARG_WITH(
  [fallback-capaths],
  [AS_HELP_STRING([--with-fallback-capaths=PATHS], [List of fallback CApaths to use for trust])]
)
AS_IF(
  [test "x$with_fallback_capaths" = "xno"],
  [],
  [test "x$with_fallback_capaths" = "xyes"],
  [AC_MSG_ERROR("--with-fallback-capaths requires a value")],
  [AC_DEFINE_UNQUOTED([FALLBACK_CAPATHS], ["$with_fallback_capaths"], [List of fallback CApaths to use for trust])]
)

AC_ARG_WITH(
  [config-file-path],
  [AS_HELP_STRING([--with-config-file-path=PATHS], [List of directories to look for the configuration file])],
  [config_file_path=${withval}],
  [config_file_path="/usr/share/defaults/swupd:/run/swupd:/etc/swupd"]
)
AC_SUBST(CONFIG_FILE_PATH, [${config_file_path}])
AC_DEFINE_UNQUOTED([CONFIG_FILE_PATH], ["${config_file_path}"], [List of directories to look for the configuration file])

AC_ARG_WITH(
  [build-number],
  [AS_HELP_STRING([--with-build-number=BUILD_NUMBER], [Build version information for this release])],
  [build_number=${withval}],
  [build_number=""]
)
AC_SUBST(BUILD_NUMBER, [${build_number}])
AC_DEFINE_UNQUOTED([BUILD_NUMBER], ["${build_number}"], [Build version information for this release])

AC_ARG_WITH(
  [system-alias-path],
  [AS_HELP_STRING([--with-system-alias-path=PATH], [Default system alias path])],
  [system_alias_path=${withval}],
  [system_alias_path="/usr/share/defaults/swupd/alias.d/"]
)
AC_SUBST(SYSTEM_ALIAS_PATH, [${system_alias_path}])
AC_DEFINE_UNQUOTED([SYSTEM_ALIAS_PATH], ["${system_alias_path}"], [system-alias-path])

AC_ARG_WITH(
  [user-alias-path],
  [AS_HELP_STRING([--with-user-alias-path=PATH], [Default user alias path])],
  [user_alias_path=${withval}],
  [user_alias_path="/etc/swupd/alias.d/"]
)
AC_SUBST(USER_ALIAS_PATH, [${user_alias_path}])
AC_DEFINE_UNQUOTED([USER_ALIAS_PATH], ["${user_alias_path}"], [user-alias-path])

AC_ARG_WITH(
  [contenturl],
  [AS_HELP_STRING([--with-contenturl=URL], [Default content url])],
  [AC_DEFINE_UNQUOTED([CONTENTURL], ["$withval"], [Default content url])]
)
AS_IF(
  [test -n "$with_contenturl" -a "$with_contenturl" != "no" -a "$with_contenturl" != "yes"],
  [CONTENTURL="$with_contenturl"],
  [test "$with_contenturl" = "no"],
  [CONTENTURL='!! Warning !! --with-contenturl not specified!'],
  [test "$with_contenturl" = "yes"],
  [CONTENTURL='!! Warning !! --with-contenturl not specified!'],
  [CONTENTURL='!! Warning !! --with-contenturl not specified!']
)

AC_ARG_WITH(
  [versionurl],
  [AS_HELP_STRING([--with-versionurl=URL], [Default version url])],
  [AC_DEFINE_UNQUOTED([VERSIONURL], ["$withval"], [Default version url])]
)
AS_IF(
  [test -n "$with_versionurl" -a "$with_versionurl" != "no" -a "$with_versionurl" != "yes"],
  [VERSIONURL="$with_versionurl"],
  [test "$with_versionurl" = "no"],
  [VERSIONURL='!! Warning !! --with-versionurl not specified!'],
  [test "$with_versionurl" = "yes"],
  [VERSIONURL='!! Warning !! --with-versionurl not specified!'],
  [VERSIONURL='!! Warning !! --with-versionurl not specified!']
)

AC_ARG_WITH(
  [formatid],
  [AS_HELP_STRING([--with-formatid=NUM], [Default format identifier])],
  [AC_DEFINE_UNQUOTED([FORMATID], ["$withval"], [Default format identifier])]
)
AS_IF(
  [test -n "$with_formatid" -a "$with_formatid" != "no" -a "$with_formatid" != "yes"],
  [FORMATID="$with_formatid"],
  [test "$with_formatid" = "no"],
  [FORMATID='!! Warning !! --with-formatid not specified!'],
  [test "$with_formatid" = "yes"],
  [FORMATID='!! Warning !! --with-formatid not specified!'],
  [FORMATID='!! Warning !! --with-formatid not specified!']
)
AS_IF(
  [test -n "$with_formatid"],
  [echo "$with_formatid" | grep -q -E '^([[1-9]][[0-9]]*|staging)$' || \
    AC_MSG_ERROR(['--with-formatid' has an invalid value ($with_formatid). Supported values are positive integers or "staging".])]
)

AC_ARG_WITH(
  [systemdsystemunitdir],
  [AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [path to systemd system service dir @<:@default=/usr/lib/systemd/system@:>@])],
  [unitpath=${withval}],
  [unitpath="$($PKG_CONFIG --variable=systemdsystemunitdir systemd)"]
)
AS_IF(
  [test -z "${unitpath}"],
  [unitpath=/usr/lib/systemd/system]
)
AC_SUBST(SYSTEMD_UNITDIR, [${unitpath}])
AC_DEFINE_UNQUOTED([SYSTEMD_UNITDIR_VAR], ["${unitpath}"], [systemdsystemunitdir])

AC_ARG_WITH(
  [pre-update],
  [AS_HELP_STRING([--with-pre-update=EXE], [Path to the pre-update executable])],
  [AC_DEFINE_UNQUOTED([PRE_UPDATE], ["$withval"], [pre-update])],
  [AC_DEFINE([PRE_UPDATE], [""], [pre-update])],
)

AC_ARG_WITH(
  [post-update],
  [AS_HELP_STRING([--with-post-update=EXE], [Path to the post-update executable])],
  [AC_DEFINE_UNQUOTED([POST_UPDATE], ["$withval"], [post-update])],
  [AC_DEFINE([POST_UPDATE], [""], [post-update])],
)

# Build variants
# default to Linux rootfs build
enable_linux_rootfs_build="yes"

# document all options for build variants
## (1) build variants
AH_TEMPLATE([SWUPD_LINUX_ROOTFS], [Enable Linux rootfs build variant])
## (2) variant features
AH_TEMPLATE([SWUPD_WITH_BINDMNTS], [cope with bind mounts over rootfs])
## (3) variant extra options
AH_TEMPLATE([MOUNT_POINT], [The mount point])
AH_TEMPLATE([STATE_DIR], [The state directory for swupd content])
AH_TEMPLATE([LOG_DIR], [Directory for swupd log files])
AH_TEMPLATE([BUNDLES_DIR], [Directory to use for bundles])
AH_TEMPLATE([CERT_PATH], [Location of update certificate])

AS_IF(
  [test "$enable_linux_rootfs_build" = "yes"],
  [AC_DEFINE([SWUPD_LINUX_ROOTFS], 1)
     AC_DEFINE([MOUNT_POINT], ["/"])
     AC_DEFINE([STATE_DIR], ["/var/lib/swupd"])
     AC_DEFINE([LOG_DIR], ["/var/log/swupd"])
     AC_DEFINE([BUNDLES_DIR], ["/usr/share/clear/bundles"])
     AC_DEFINE_UNQUOTED([CERT_PATH], ["$cert_path"])],
  [AC_MSG_ERROR([Unknown build variant])]
)

AC_SUBST([cert_path], ["$cert_path"])

AC_CONFIG_FILES([Makefile data/swupd-update.service docs/swupd-alias.7.rst])
AC_REQUIRE_AUX_FILE([tap-driver.sh])
AC_OUTPUT

AC_MSG_NOTICE([

------------
swupd-client
------------

Configuration to build swupd-client:

  System alias path:            ${system_alias_path}
  User alias path:              ${user_alias_path}
  Content URL:                  ${CONTENTURL}
  Version URL:                  ${VERSIONURL}
  Format Identifier:            ${FORMATID}
  Signature verification:       ${SIGVERIFICATION}
  Third party:                  ${THIRDPARTY}
  Update certificate path:      ${cert_path}
  Configuration file path(s):   ${config_file_path}
  Use bzip compression:         ${BZIP}
  Run Tests:                    ${TESTS}
  Use extended file attributes  ${XATTR}
  Use --selinux option for tar  ${TARSELINUX}
  External modules support      ${external_modules}
])
