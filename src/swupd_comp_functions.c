/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2020 Intel Corporation.
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

#define _GNU_SOURCE

#include <string.h>

#include "3rd_party_repos.h"
#include "swupd.h"

/**
 * All functions in this file should match this type definition:
 * typedef int (*comparison_fn_t)(const void *a, const void *b);
 *
 * They should behave like this (similar to strcmp):
 * - return 0 if a is equal to b
 * - return any number "< 0" if a is lower than b
 * - return any number "> 0" if a is bigger than b
 *
 * Naming convention:
 *
 * If both elements being compared are the same type and subtype:
 *   cmp_<type>_<subtype>
 *
 * If both elements being compared are different type or subtype:
 *   cmp_<type1>_<subtype1>_<type2>_<subtype2>
 */

int cmp_string_filerecord_filename(const void *a, const void *b)
{
	return strcmp(*(const char **)a, ((struct filerecord *)b)->filename);
}

int cmp_file_filename(const void *a, const void *b)
{
	return strcmp(((struct file *)a)->filename, ((struct file *)b)->filename);
}

int cmp_file_filename_ptr(const void *a, const void *b)
{
	return strcmp((*(struct file **)a)->filename, (*(struct file **)b)->filename);
}

int cmp_filerecord_filename(const void *a, const void *b)
{
	return strcmp(((struct filerecord *)a)->filename, ((struct filerecord *)b)->filename);
}

int cmp_file_hash(const void *a, const void *b)
{
	return strcmp(((struct file *)a)->hash, ((struct file *)b)->hash);
}

int cmp_manifest_component(const void *a, const void *b)
{
	return strcmp(((struct manifest *)a)->component, ((struct manifest *)b)->component);
}

int cmp_subscription_component(const void *a, const void *b)
{
	return strcmp(((struct sub *)a)->component, ((struct sub *)b)->component);
}

int cmp_manifest_component_string(const void *a, const void *b)
{
	return strcmp(((struct manifest *)a)->component, (const char *)b);
}

int cmp_file_filename_string(const void *a, const void *b)
{
	return strcmp(((struct file *)a)->filename, (const char *)b);
}

int cmp_sub_component_string(const void *a, const void *b)
{
	return strcmp(((struct sub *)a)->component, (const char *)b);
}

int cmp_file_filename_reverse(const void *a, const void *b)
{
	int ret;

	ret = cmp_file_filename(a, b);
	return -ret;
}

int cmp_file_hash_last_change(const void *a, const void *b)
{
	struct file *file1 = (struct file *)a;
	struct file *file2 = (struct file *)b;
	int comp;

	comp = strcmp(file1->hash, file2->hash);
	if (comp != 0) {
		return comp;
	}

	/* they have the same hash, now let's check the version */
	return file1->last_change - file2->last_change;
}

int cmp_file_filename_is_deleted(const void *a, const void *b)
{
	struct file *A, *B;
	int ret;
	A = (struct file *)a;
	B = (struct file *)b;

	ret = strcmp(A->filename, B->filename);
	if (ret) {
		return ret;
	}
	if (A->is_deleted > B->is_deleted) {
		return 1;
	}
	if (A->is_deleted < B->is_deleted) {
		return -1;
	}

	return 0;
}

#ifdef THIRDPARTY

int cmp_repo_name_string(const void *a, const void *b)
{
	return strcmp(((struct repo *)a)->name, (const char *)b);
}

int cmp_repo_url_string(const void *a, const void *b)
{
	return strcmp(((struct repo *)a)->url, (const char *)b);
}

#endif
