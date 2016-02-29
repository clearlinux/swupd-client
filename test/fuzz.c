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
#include <bsdiff.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>

#include "swupd.h"
#include <pthread.h>

#define THREAD_COUNT 12

#define OUTDIR "fuzzout/"

// NOTE: Slot 0 is never used.
char *from_files[THREAD_COUNT + 1];
char *to_files[THREAD_COUNT + 1];

static void banner(void)
{
	printf("Usage:\n\tfuzz <file1> <file2>...\n");
	printf("\n");
	exit(0);
}

static void corrupt_file(int thread)
{
	struct stat statb;
	FILE *inp, *outp;
	char *buf;
	char filename[PATH_MAXLEN];
	int ret;

	int r, b;

	sprintf(filename, OUTDIR "tempfile.%i", thread);
	ret = stat(filename, &statb);
	if (ret) {
		printf("Stat failure\n");
		exit(0);
	}
	buf = malloc(statb.st_size);
	assert(buf != NULL);
	inp = fopen(filename, "r");
	assert(inp != NULL);
	ret = fread(buf, statb.st_size, 1, inp);
	fclose(inp);
	if (ret != 1) {
		return;
	}

	/* corrupt at least 1 byte, with 1/2 chance of doing one more byte etc */
	do {
		r = rand() % statb.st_size;
		b = rand() & 255;

		buf[r] = b;
	} while (rand() & 1);

	sprintf(filename, OUTDIR "corrupt.%i", thread);
	outp = fopen(filename, "w");
	assert(outp != NULL);
	fwrite(buf, statb.st_size, 1, outp);
	fclose(outp);
}

static int check_valgrind(int thread)
{
	FILE *file;
	char line[PATH_MAXLEN];
	char *ret;

	sprintf(line, OUTDIR "valgrind.log.%i", thread);
	file = fopen(line, "r");
	while (!feof(file)) {
		ret = fgets(line, PATH_MAXLEN, file);
		if (ret == NULL) {
			fclose(file);
			return 1;
		}
		if (strstr(line, "ERROR SUMMARY: 0 errors")) {
			fclose(file);
			return 0;
		}
	}
	fclose(file);
	return 1;
}

static void *do_fuzz(void *data)
{
	int i = 1000;
	int thread = (unsigned long)data;
	char filename[PATH_MAXLEN];
	int ret;

	printf("Thread %i: Fuzzing %s vs %s\n", thread, from_files[thread], to_files[thread]);

	sprintf(filename, OUTDIR "tempfile.%i", thread);

	make_bsdiff_delta(from_files[thread], to_files[thread], filename, (rand() % 10 > 8));

	apply_bsdiff_delta(from_files[thread], OUTDIR "testout", filename);

	/* do a large number of runs and then exit, so that we can randomly pick files from an outer script */
	while (i-- > 0) {
		char *command;

		/* step 3: randomly corrupt the "tempfile" into a "corrupt" file*/
		corrupt_file(thread);
		/* step 4: run valgrind on the bspatch program with the "corrupt" file */
		if (asprintf(&command, "valgrind --leak-check=no --log-file=" OUTDIR "valgrind.log.%i bspatch %s " OUTDIR "output.%i " OUTDIR "corrupt.%i > /dev/null",
			     thread, from_files[thread], thread, thread) <= 0) {
			assert(0);
		}

		ret = system(command);
		free(command);
		/* step 5: check valgrind output for failures, if failed, copy corrupt to "failed" dir */

		if (check_valgrind(thread)) {
			int t;
			char *command2;

			t = time(NULL);
			if (asprintf(&command2, "mv " OUTDIR "corrupt.%i " OUTDIR "failed/corrupt.%i-%i",
				     thread, thread, t) <= 0) {
				assert(0);
			}
			ret = system(command2);
			assert(ret == 0);
			free(command2);
			if (asprintf(&command2, "mv " OUTDIR "valgrind.log.%i " OUTDIR "failed/valgrind.%i-%i",
				     thread, thread, t) <= 0) {
				assert(0);
			}
			ret = system(command2);
			assert(ret == 0);
			free(command2);
			if (asprintf(&command2, "cp %s " OUTDIR "failed/original.%i-%i",
				     from_files[thread], thread, t) <= 0) {
				assert(0);
			}
			ret = system(command2);
			assert(ret == 0);
			free(command2);
			printf("x");
			sleep(1); /* make sure no duplicates exist */
		} else if (i % 10 == 0) {
			printf(".");
		}
		fflush(stdout);
	}
	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t threads[THREAD_COUNT];
	int i;
	int p;
	int q;

	if (argc < 2) {
		banner();
	}

	srand(time(NULL));

	/* step 1: make a "failed" directory */
	mkdir(OUTDIR, S_IRWXU);
	mkdir(OUTDIR "failed", S_IRWXU);

	while (1) {

		for (i = 1; i <= THREAD_COUNT; i++) {
			p = 0;
			q = 0;
			while (p == 0 || q == 0) {
				p = (rand() % argc);
				q = (rand() % argc);
			}
			from_files[i] = argv[p];
			to_files[i] = argv[q];
		}

		for (i = 1; i <= THREAD_COUNT; i++) {
			pthread_create(&threads[i], NULL, do_fuzz, (void *)(unsigned long)i);
		}

		pthread_exit(NULL);

		printf("\n");
	}
	return 0;
}
