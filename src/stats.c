/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2016 Intel Corporation.
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
 *   Authors:
 *         Arjan van de Ven <arjan@linux.intel.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "swupd.h"

int swupd_stats[8];

void print_statistics(int version1, int version2)
{
	info("\n");
	info("Statistics for going from version %i to version %i:\n\n", version1, version2);
	info("    changed bundles   : %i\n", swupd_stats[5]);
	info("    new bundles       : %i\n", swupd_stats[3]);
	info("    deleted bundles   : %i\n\n", swupd_stats[4]);
	info("    changed files     : %i\n", swupd_stats[2]);
	info("    new files         : %i\n", swupd_stats[0]);
	info("    deleted files     : %i\n", swupd_stats[1]);
	info("\n");
}
