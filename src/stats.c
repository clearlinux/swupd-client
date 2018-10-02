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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "stats.h"

int swupd_stats[STATISTICS_SIZE];

void print_statistics(int version1, int version2)
{
	printf("\n");
	printf("Statistics for going from version %i to version %i:\n\n", version1, version2);
	printf("    changed bundles   : %i\n", swupd_stats[CHANGED_BUNDLES]);
	printf("    new bundles       : %i\n", swupd_stats[NEW_BUNDLES]);
	printf("    deleted bundles   : %i\n\n", swupd_stats[DELETED_BUNDLES]);
	printf("    changed files     : %i\n", swupd_stats[CHANGED_FILES]);
	printf("    deleted files     : %i\n", swupd_stats[DELETED_FILES]);
	printf("\n");
}
