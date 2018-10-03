/*
 *   Software Updater - client side
 *
 *      Copyright © 2016 Intel Corporation.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 2 or later of the License.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __INCLUDE_GUARD_SWUPD_BUILD_OPTS_H
#define __INCLUDE_GUARD_SWUPD_BUILD_OPTS_H

#include "config.h"

#ifdef SWUPD_WITH_BZIP2
#define OPT_BZIP2 "+BZIP2"
#else
#define OPT_BZIP2 "-BZIP2"
#endif

#ifdef SIGNATURES
#define OPT_SIGNATURES "+SIGVERIFY"
#else
#define OPT_SIGNATURES "-SIGVERIFY"
#endif

#ifdef COVERAGE
#define OPT_COVERAGE "+COVERAGE"
#else
#define OPT_COVERAGE "-COVERAGE"
#endif

#ifdef SWUPD_WITH_BSDTAR
#define OPT_BSDTAR "+BSDTAR"
#else
#define OPT_BSDTAR "-BSDTAR"
#endif

#ifdef SWUPD_WITH_XATTRS
#define OPT_XATTRS "+XATTRS"
#else
#define OPT_XATTRS "-XATTRS"
#endif

#ifdef SWUPD_TAR_SELINUX
#define OPT_SELINUX "+TAR_SELINUX"
#else
#define OPT_SELINUX "-TAR_SELINUX"
#endif

#ifdef OS_IS_STATELESS
#define OPT_STATELESS "+STATELESS"
#else
#define OPT_STATELESS "-STATELESS"
#endif

#ifdef CONTENTURL
#define OPT_C_URL CONTENTURL
#else
#define OPT_C_URL "no default set"
#endif

#ifdef VERSIONURL
#define OPT_V_URL VERSIONURL
#else
#define OPT_V_URL "no default set"
#endif

#ifdef FORMATID
#define OPT_FORMAT FORMATID
#else
#define OPT_FORMAT "no default set"
#endif

#ifdef PRE_UPDATE
#define OPT_PRE PRE_UPDATE
#else
#define OPT_PRE "no default set"
#endif

#ifdef POST_UPDATE
#define OPT_POST POST_UPDATE
#else
#define OPT_POST "no default set"
#endif

#define BUILD_OPTS \
	OPT_BZIP2 " " OPT_SIGNATURES " " OPT_COVERAGE " " OPT_BSDTAR " " OPT_XATTRS " " OPT_SELINUX " " OPT_STATELESS

#define BUILD_CONFIGURE                                       \
	"mount point                  " MOUNT_POINT "\n"      \
	"state directory              " STATE_DIR "\n"        \
	"bundles directory            " BUNDLES_DIR "\n"      \
	"certificate path             " CERT_PATH "\n"        \
	"fallback certificate path    " FALLBACK_CAPATHS "\n" \
	"content URL                  " OPT_C_URL "\n"        \
	"version URL                  " OPT_V_URL "\n"        \
	"format ID                    " OPT_FORMAT "\n"       \
	"pre-update hook              " OPT_PRE "\n"          \
	"post-update hook             " OPT_POST "\n"

#endif
