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

#define BUILD_OPTS \
	OPT_BZIP2 " " OPT_SIGNATURES " " OPT_COVERAGE " " OPT_BSDTAR

#endif
