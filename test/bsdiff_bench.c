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
 *         Eric Lapuyade <eric.lapuyade@intel.com>
 *         cguiraud <christophe.guiraud@intel.com>
 *         Timothy C. Pepper <timothy.c.pepper@linux.intel.com>
 *         Arjan van de Ven <arjan@linux.intel.com>
 *
 */

#define _GNU_SOURCE
#include <assert.h>
#include <bsdiff.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "swupd.h"

double a[5], b[5];
int size[5];
double count[5];

int main(int UNUSED_PARAM argc, char UNUSED_PARAM **argv)
{
	int ret;
	int algo;
	struct timeval before, after;
	struct stat st1, st2;
	struct file *file1, *file2;

	ret = stat(argv[1], &st1);
	if (ret) {
		exit(0);
	}
	ret = stat(argv[2], &st2);
	if (ret) {
		exit(0);
	}

	file1 = calloc(1, sizeof(struct file));
	assert(file1);
	file1->use_xattrs = true;
	file1->filename = strdup(argv[2]);

	file2 = calloc(1, sizeof(struct file));
	assert(file2);
	file2->use_xattrs = true;
	file1->filename = strdup("result");

	populate_file_struct(file1, argv[2]);
	ret = compute_hash(file1, argv[2]);
	if ((ret != 0) || hash_is_zeros(file1->hash)) {
		printf("Hash computation failed\n");
		exit(0);
	}

	for (algo = 0; algo < BSDIFF_ENC_LAST; algo++) {
		struct stat bu;
		int i;
		time_t start;

		unlink("output.bsdiff");
		make_bsdiff_delta(argv[1], argv[2], "output.bsdiff", algo);

		stat("output.bsdiff", &bu);

		start = time(NULL);
		gettimeofday(&before, NULL);
		for (i = 0; i < 10000; i++) {
			ret = apply_bsdiff_delta(argv[1], "result", "output.bsdiff");
			if (i > 500 && time(NULL) - start > 5) {
				break;
			}
		}
		gettimeofday(&after, NULL);
		populate_file_struct(file1, "result");
		ret = compute_hash(file2, "result");
		if ((ret != 0) || hash_is_zeros(file2->hash)) {
			printf("Hash computation failed\n");
			exit(0);
		}
		if (!hash_equal(file1->hash, file2->hash)) {
			printf("Hash mismatch for algorithm %i \n", algo);
			exit(0);
		}
		unlink("result");

		b[algo] = before.tv_sec + before.tv_usec / 1000000.0;
		a[algo] = after.tv_sec + after.tv_usec / 1000000.0;
		count[algo] = i / 5000.0;
		size[algo] = bu.st_size;
	}

	printf("file, %s, orgsize, %i, best, %i, unc, %i, %5.3f, bzip, %i, %5.3f, gzip, %i, %5.3f, xz, %i, %5.3f, zeros, %i, %5.3f\n",
	       argv[1], (int)(st1.st_size + st2.st_size) / 2, size[0],
	       size[1], (a[1] - b[1]) / count[1],
	       size[2], (a[2] - b[2]) / count[2],
	       size[3], (a[3] - b[3]) / count[3],
	       size[4], (a[4] - b[4]) / count[4],
	       size[5], (a[5] - b[5]) / count[5]);

	return ret;
}
