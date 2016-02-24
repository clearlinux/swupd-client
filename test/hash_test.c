/*
 *   Software Updater - client side
 *
 *      Copyright © 2013-2016 Intel Corporation.
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
 *         Christophe Guiraud <christophe.guiraud@intel.com>
 *         Eric Lapuyade <eric.lapuyade@intel.com>
 *         Timothy C. Pepper <timothy.c.pepper@linux.intel.com>
 *
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h> 
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <sys/types.h> 
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h> 
#include <sys/xattr.h>
#include <errno.h>
#include <assert.h>

#include "swupd.h"
#include "xattrs.h"

static void print_usage(void);

#define HMAC_SHA1 	1
#define HMAC_SHA256	4
#define HMAC_SHA512	8

struct hmac_sha_stat {
	uint64_t st_mode;
	uint64_t st_uid;
	uint64_t st_gid;
	uint64_t st_rdev;
	uint64_t st_size;
};

static char *bin2hex(const unsigned char *buf, size_t len)
{
	static const char x_digit[] = "0123456789ABCDEF";
	size_t i;
	char *hex;
	char *tmp;

	hex = calloc(1, (len * 2) + 1);
	if (hex == NULL) {
		printf("Error: Memory Allocation\n");
		return NULL;
	}

	tmp = hex;
	for (i = 0; i < len; i++, buf++) {
		*tmp++ = x_digit[(*buf >> 4)];
		*tmp++ = x_digit[(*buf & 0x0F)];
	}

	return hex;
}

static char *hmac_sha_for_data(int method, const unsigned char *key, size_t key_len, const unsigned char *data, size_t data_len)
{
	unsigned char digest[EVP_MAX_MD_SIZE];
	unsigned int digest_len = 0;
	char *digest_str;
	const EVP_MD* evp_method;

	switch (method) {
		case HMAC_SHA1:
			evp_method = EVP_sha1();
			break;
		case HMAC_SHA256:
			evp_method = EVP_sha256();
			break;
		case HMAC_SHA512:
			evp_method = EVP_sha512();
			break;
		default:
			return NULL;
	}

	if ((data == NULL) || (data_len == 0))
		return NULL;

	if (HMAC(evp_method, (const void *)key, key_len, data, data_len, digest, &digest_len) == NULL)
		return NULL;

	digest_str = bin2hex(digest, digest_len);
	if (digest_str == NULL)
		return NULL;

	return digest_str;
}

static char *hmac_sha_for_string(int method, const unsigned char *key, size_t key_len, const char *str)
{
	if (str == NULL)
		return NULL;

	return hmac_sha_for_data(method, key, key_len, (const unsigned char *)str, strlen(str));
}

static void print_xattr_list(const unsigned char *list, size_t len)
{
	char *hexstr;

	hexstr = bin2hex(list, len);
	if (!hexstr) assert(0);

	printf("- X-Attribute-Blob: Length=[%lu], Value=[0x%s]\n", (long unsigned int)len, hexstr);
	free(hexstr);
}

static void hmac_compute_key(int method, const char *file,
			     const struct hmac_sha_stat *updt_stat,
			     char **key, size_t *key_len, int verbose)
{
	char *xattrs_blob = NULL;
	size_t len = 0;
	char* key_str;

	*key_len = 0;

	xattrs_get_blob(file, &xattrs_blob, &len);

	if ((xattrs_blob != NULL) && (len != 0)) {
		*key = hmac_sha_for_data(method, (const unsigned char *)updt_stat,
					 sizeof(*updt_stat),
					 (const unsigned char *)xattrs_blob,
					 len);
		if (*key != NULL) {
			*key_len = strlen((const char*)*key);
			if (verbose) {
				print_xattr_list((const unsigned char*)xattrs_blob, len);
				printf("- Hash Key: Length=[%lu], Value=[0x%s]\n", (long unsigned int)(*key_len), *key);
			}
		} else {
			*key_len = 0;
			printf("- Hash Key: Length=[%lu], Value=(null)\n", (long unsigned int)(*key_len));
			fprintf(stderr, "computing hash key: error while computing key for file %s: %s\n",
				file, strerror(errno));
		}


		if (len != 0)
			free(xattrs_blob);
	} else {
		*key = calloc(1, sizeof(*updt_stat));
		if (*key != NULL) {
			*key_len = sizeof(*updt_stat);
			memcpy(*key, updt_stat, *key_len);

			if (verbose) {
				key_str = bin2hex((const unsigned char*)key, *key_len);
				if (!key_str) assert(0);
				printf("- Stat Key: Length=[%lu], Value=[0x%s]\n", (long unsigned int)(*key_len), (const char *)key_str);
				free(key_str);
			}
		} else {
			fprintf(stderr, "computing hash key: error while computing key for file %s: %s\n",
				file, strerror(errno));
			return;
		}
	}
}

static void compute_hmac_sha(int method, char *dirpath, int verbose, unsigned int *hashcount)
{ 
	DIR *dir;
	struct dirent *entry;
	struct stat stat;
	char *statfile = NULL;
	int ret;
	unsigned char *blob;
	FILE *fl;
	struct hmac_sha_stat tfstat;
	char link[PATH_MAXLEN];
	char *hash = NULL;
	char *key = NULL;
	size_t key_len;

	dir = opendir(dirpath);
	if (dir == NULL)
		return;

	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		if (asprintf(&statfile, "%s/%s", dirpath, entry->d_name) <= 0)
			assert(0);

		if (entry->d_type == DT_DIR)
			compute_hmac_sha(method, statfile, verbose, hashcount);

		if (verbose)
			printf("File: %s\n", statfile);

		memset(&stat, 0, sizeof(stat));
		ret = lstat(statfile, &stat);
		if (ret < 0) {
			fprintf(stderr, "computing hash: stat error for file %s: %s\n", statfile, strerror(errno));
			goto on_exit;
		}

		tfstat.st_mode = stat.st_mode;
		tfstat.st_uid = stat.st_uid;
		tfstat.st_gid = stat.st_gid;
		tfstat.st_rdev = stat.st_rdev;
		tfstat.st_size = stat.st_size;

		if (S_ISLNK(stat.st_mode)) {
			memset(link, 0, PATH_MAXLEN);
			if ((ret = readlink(statfile, link, PATH_MAXLEN - 1)) < 0) {
				printf("readlink error %i - %i / %s\n", ret, errno, strerror(errno));
				goto on_exit;
			}
			hmac_compute_key(method, statfile, &tfstat, &key, &key_len, verbose);
			if (key != NULL) {
				hash = hmac_sha_for_string(method, (const unsigned char *)key, key_len, link);
				(*hashcount)++;
			}
		} else if (S_ISDIR(stat.st_mode)) {
			tfstat.st_size = 0;
			hmac_compute_key(method, statfile, &tfstat, &key, &key_len, verbose);
			if (key != NULL) {
				hash = hmac_sha_for_string(method, (const unsigned char *)key, key_len, statfile);
				(*hashcount)++;
			}
		} else { /* this is a regular file */
			if ((fl = fopen(statfile, "r")) == NULL) {
				fprintf(stderr, "computing hash: file open error for file %s: %s\n", statfile, strerror(errno));
				goto on_exit;
			}

			blob = mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, fileno(fl), 0);
			hmac_compute_key(method, statfile, &tfstat, &key, &key_len, verbose);
			if (key != NULL) {
				hash = hmac_sha_for_data(method, (const unsigned char *)key, key_len, blob, stat.st_size);
				(*hashcount)++;
			}
			munmap(blob, stat.st_size);
			fclose(fl);
		}

		if (!hash) assert(0);
		if (verbose)
			printf("- Hash: 0x%s\n", hash);

		free(key);
		key = NULL;
		free(hash);
		hash = NULL;
		free(statfile);
		statfile = NULL;
	}

on_exit:
	free(key);
	free(hash);
	free(statfile);
	closedir(dir);
} 

static void bench_compute_hmac_sha(int method, char *dirpath, int verbose)
{
	struct timeval start;
	struct timeval end;
	double diff_us;
	
	unsigned int hashcount = 0;

	gettimeofday(&start, NULL);

	compute_hmac_sha(method, dirpath, verbose, &hashcount);

	gettimeofday(&end, NULL);

	diff_us = (end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);
	printf("...Processing time = %f seconds - Computed hash count = %u\n", diff_us / 1000000., hashcount);
}

static void bench_hmac_sha(int argc, char *argv[])
{
	int verbose = 0;

	if (argc > 3) {
		if ((argc > 4) && (strcmp(argv[4], "-v") == 0))
			verbose = 1;

		if (strcmp(argv[2], "hmac-sha1") == 0) {
		  	printf("[bench hmac sha1]\n");
			return bench_compute_hmac_sha(HMAC_SHA1, argv[3], verbose);
		} else if (strcmp(argv[2], "hmac-sha256") == 0) {
		  	printf("[bench hmac sha256]\n");
			return bench_compute_hmac_sha(HMAC_SHA256, argv[3], verbose);
		} else if (strcmp(argv[2], "hmac-sha512") == 0) {
		  	printf("[bench hmac sha512]\n");
			return bench_compute_hmac_sha(HMAC_SHA512, argv[3], verbose);
		}
	}

	print_usage();
}

static void *multithreads_do_hash(void *threadid)
{
	long tid;
	char *hash;

	tid = (long)threadid;
	hash = hmac_sha_for_string(HMAC_SHA256, (const unsigned char *)&threadid, sizeof(threadid), "The quick brown fox jumps over the lazy dog");
	if (!hash) assert(0);

	printf("multithreads: do_hash - Thread[#%ld] - hash = %s\n", tid, hash);

	free(hash);

	pthread_exit(NULL);

	return NULL;
}

#define NUM_THREADS	4
static void multithreads(void)
{
	pthread_t threads[NUM_THREADS];
	int ret;
	long t;

	for (t = 1; t < NUM_THREADS + 1; t++) {
		printf("multithreads: creating thread %ld/%d\n", t, NUM_THREADS);
		ret = pthread_create(&threads[t], NULL, multithreads_do_hash, (void *)t);
		if (ret) {
			printf("ERROR; return code from pthread_create() is %d\n", ret);
			exit(-1);
		}
	}
	
	pthread_exit(NULL);
}

static void xattrs_set_cmd(char *file, char *attr_name, char *attr_value)
{
	int ret;

	ret = setxattr(file, attr_name, (void*)&attr_value,
		       strlen(attr_value) + 1, 0);
	if (ret < 0)
		printf("Set Extended attribute for file %s FAILED: %s\n", file,
		       strerror(errno));
	else
		printf("Set Extended attribute for file %s SUCCESS\n", file);
}

static void xattrs_get_blob_cmd(char *file)
{
	char *xattrs_blob = NULL;
	size_t len = 0;

	xattrs_get_blob(file, &xattrs_blob, &len);

	if ((xattrs_blob != NULL) && (len != 0)) {
		printf("Extended attributes data blob for file %s:\n", file);
		print_xattr_list((const unsigned char*)xattrs_blob, len);
	} else {
		printf("Extended attributes data blob for file %s is EMPTY\n",
		       file);
	}

	if (len != 0)
		free(xattrs_blob);
}

static void xattrs_copy_cmd(char *src, char *dst)
{
	printf("Attempt to copy extended attributes from %s to %s\n", src, dst);
	xattrs_copy(src, dst);
}

static void xattrs_test(char *dirpath)
{
	char *src = NULL;
	char *dst = NULL;
	FILE *file = NULL;
	int ret;

	/* Create 2 files in the given working directory,
	   Set an extended attribute to the first file.
	   Copy the first file extended attributes to the other one.
	   Get the extended attributes data blob for both file and compare.
	   The test is sucessful if extended attributes data blobs are identical.
	*/

	printf("Extended attribute test working dir %s:\n", dirpath);

	/* step 4: run valgrind on the patch program with the "corrupt" file */
	if (asprintf(&src, "%s/testfile1.txt", dirpath) <= 0)
		assert(0);

	if (asprintf(&dst, "%s/testfile2.txt", dirpath) <= 0)
		assert(0);

	file = fopen(src, "w+");
	if (file != NULL) {
		fwrite("The quick brown fox",
		       sizeof(char), sizeof("The quick brown fox"), file);
		fclose(file);
	}

	file = fopen(dst, "w+");
	if (file != NULL) {
		fwrite("The quick brown fox",
		       sizeof(char), sizeof("The quick brown fox"), file);
		fclose(file);
	}

	ret = setxattr(src, "user.xattr_test", (void*)"xattr_test_val",
		       strlen("xattr_test_val") + 1, 0);
	if (ret < 0)
		printf("Set Extended attribute for file %s FAILED: %s\n", src,
		       strerror(errno));
	else
		printf("Set Extended attribute for file %s SUCCESS\n", src);

	xattrs_copy(src, dst);

	if (xattrs_compare(src, dst) != 0)
		printf("TEST FAILURE\n");
	else
		printf("TEST SUCCESS\n");

	if (src != NULL)
		unlink(src);

	if (dst != NULL)
		unlink(dst);

	free(src);
	free(dst);
}

static void xattrs(int argc, char *argv[])
{
  printf("[xattrs argc %d]\n", argc);

	if (argc > 3) {
		if (strcmp(argv[2], "set") == 0) {
			if (argc != 6)
				return print_usage();
			printf("[set]\n");
			return xattrs_set_cmd(argv[3], argv[4], argv[5]);
		} else if (strcmp(argv[2], "get_blob") == 0) {
			if (argc != 4)
				return print_usage();
			printf("[get_blob]\n");
			return xattrs_get_blob_cmd(argv[3]);
		} else if (strcmp(argv[2], "copy") == 0) {
			if (argc != 5)
				return print_usage();
			printf("[copy]\n");
			return xattrs_copy_cmd(argv[3], argv[4]);
		}  else if (strcmp(argv[2], "test") == 0) {
			if (argc != 4)
				return print_usage();
			printf("[test]\n");
			return xattrs_test(argv[3]);
		}
	}

	print_usage();
}

static void print_usage(void)
{
	printf("Usage:\n");
	printf("	hash_test multithreads\n");
	printf("	hash_test bench hmac-sha1 <path> [-v]\n");
	printf("	hash_test bench hmac-sha256 <path> [-v]\n");
	printf("	hash_test bench hmac-sha512 <path> [-v]\n");
	printf("	hash_test xattrs set <filepath> <user.attr_name> <attr_value>\n");
	printf("	hash_test xattrs get_blob <filepath>\n");
	printf("	hash_test xattrs copy <src_filepath> <dst_filepath>\n");
	printf("	hash_test xattrs test <R/W working path>\n");
	printf("	hash_test help\n");
}

int main(int argc, char *argv[])
{
	if (argc > 1) {
		if (strcmp(argv[1], "multithreads") == 0) {
			printf("[multithreads]\n");
			multithreads();
			return 0;
		} else if (strcmp(argv[1], "bench") == 0) {
			bench_hmac_sha(argc, argv);
			return 0;
		} else if (strcmp(argv[1], "xattrs") == 0) {
			xattrs(argc, argv);
			return 0;
		}
	}

	print_usage();

	return 0;
}
