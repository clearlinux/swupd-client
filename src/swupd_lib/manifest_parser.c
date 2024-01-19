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

#include <cpuid.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "manifest.h"
#include "swupd.h"

#define MANIFEST_LINE_MAXLEN (PATH_MAX * 2)
#define MANIFEST_HEADER "MANIFEST\t"

const char AVX2_SKIP_FILE[] = "/etc/clear/elf-replace-avx2";
const char AVX512_SKIP_FILE[] = "/etc/clear/elf-replace-avx512";

/* Below generated with the following:

    unsigned char lt[256] = ".acdefghijklmnopqrtuvwxyzABDEFGHIJKLMNOPQRSTUVWXYZ0123456789!#^*bsC";
    unsigned char ltr[256] = {0};
    for (unsigned i = 0; i < 256; i++) {
	ltr[lt[i]] = i;
    }
    ltr[0] = 0;
    ltr['b'] = ltr['s'] = ltr['C'] = 0;

    printf("static unsigned char OPTIMIZED_BITMASKS[256] = {\n");
    for (int i = 0; i < 256; i++) {
	if (i < 255) {
		printf("0x%X,\n", ltr[i]);
	} else {
		printf("0x%X\n", ltr[i]);
	}
    }
    printf("};\n");
 */

/* Changes to the OPTIMIZED_BITMASKS array require a format bump and corresponding mixer change */
static unsigned char OPTIMIZED_BITMASKS[256] = { 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X3C, 0X0, 0X3D, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X3F, 0X0, 0X0, 0X0, 0X0, 0X0, 0X32, 0X33, 0X34, 0X35, 0X36, 0X37, 0X38, 0X39, 0X3A, 0X3B, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X19, 0X1A, 0X0, 0X1B, 0X1C, 0X1D, 0X1E, 0X1F, 0X20, 0X21, 0X22, 0X23, 0X24, 0X25, 0X26, 0X27, 0X28, 0X29, 0X2A, 0X2B, 0X2C, 0X2D, 0X2E, 0X2F, 0X30, 0X31, 0X0, 0X0, 0X0, 0X3E, 0X0, 0X0, 0X1, 0X0, 0X2, 0X3, 0X4, 0X5, 0X6, 0X7, 0X8, 0X9, 0XA, 0XB, 0XC, 0XD, 0XE, 0XF, 0X10, 0X11, 0X0, 0X12, 0X13, 0X14, 0X15, 0X16, 0X17, 0X18, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0, 0X0 };

/* Any changes to LOOKUP_OPTIMIZED_BITMASKS must match mixer,
   requires format bump */
static uint64_t LOOKUP_OPTIMIZED_BITMASKS[256] = {
	[SSE_0] = (SSE << 8) | SSE,
	[SSE_1] = (SSE << 8) | SSE | AVX2,
	[SSE_2] = (SSE << 8) | SSE | AVX512,
	[SSE_3] = (SSE << 8) | SSE | AVX2 | AVX512,
	[AVX2_1] = (AVX2 << 8) | SSE | AVX2,
	[AVX2_3] = (AVX2 << 8) | SSE | AVX2 | AVX512,
	[AVX512_2] = (AVX512 << 8) | SSE | AVX512,
	[AVX512_3] = (AVX512 << 8) | SSE | AVX2 | AVX512,
	[SSE_4] = (SSE << 8) | SSE | APX,
	[SSE_5] = (SSE << 8) | SSE | AVX2 | APX,
	[SSE_6] = (SSE << 8) | SSE | AVX512 | APX,
	[SSE_7] = (SSE << 8) | SSE | AVX2 | AVX512 | APX,
	[AVX2_5] = (AVX2 << 8) | SSE | AVX2 | APX,
	[AVX2_7] = (AVX2 << 8) | SSE | AVX2 | AVX512 | APX,
	[AVX512_6] = (AVX512 << 8) | SSE | AVX512 | APX,
	[AVX512_7] = (AVX512 << 8) | SSE | AVX2 | AVX512 | APX,
	[APX_4] = (APX << 8) | SSE | APX,
	[APX_5] = (APX << 8) | SSE | AVX2 | APX,
	[APX_6] = (APX << 8) | SSE | AVX512 | APX,
	[APX_7] = (APX << 8) | SSE | AVX2 | AVX512 | APX,
};

/* some CPUs support avx512 but it's not helpful for performance ... skip avx512 there */
int avx512_is_unhelpful(void)
{
	uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
	uint32_t model;

	/* currently only Intel cpus are on the list */
	if (!__builtin_cpu_is("intel")) {
		return 0;
	}

	__cpuid(1, eax, ebx, ecx, edx);
	model = (eax >> 4) & 0xf;
	model += ((eax >> (16 - 4)) & 0xf0);

	/* tigerlake */
	if (model == 0x8C) {
		return 1;
	}
	if (model == 0x8D) {
		return 1;
	}

	return 0;
}

uint64_t get_opt_level_mask(void)
{
	char *avx2_skip_file = NULL;
	char *avx512_skip_file = NULL;
	uint64_t opt_level_mask = 0;

	avx2_skip_file = sys_path_join("%s/%s", globals.path_prefix, AVX2_SKIP_FILE);
	avx512_skip_file = sys_path_join("%s/%s", globals.path_prefix, AVX512_SKIP_FILE);

	if (__builtin_cpu_supports("avx512vl")) {
		opt_level_mask = (AVX512 << 8) | AVX512 | AVX2 | SSE;
	} else if (__builtin_cpu_supports("avx2") && __builtin_cpu_supports("fma")) {
		opt_level_mask = (AVX2 << 8) | AVX2 | SSE;
	} else {
		/* SSE is zero but keep the same logic as others */
		opt_level_mask = (SSE << 8) | SSE;
	}

	if ((opt_level_mask >> 8) & AVX512 && avx512_is_unhelpful()) {
		/* Pretend we are AVX2 only */
		opt_level_mask = (AVX2 << 8) | AVX2 | SSE;
	}

	if (!access(avx512_skip_file, F_OK)) {
		if ((opt_level_mask >> 8) & AVX512) {
			/* Downgrade the entire system to AVX2 */
			opt_level_mask = (AVX2 << 8) | AVX2 | SSE;
		}
	}

	if (!access(avx2_skip_file, F_OK)) {
		if ((opt_level_mask >> 8) & AVX2) {
			/* Downgrade the entire system to SSE */
			opt_level_mask = (SSE << 8) | SSE;
		} else {
			/* Otherwise just remove AVX2 support from the flags */
			opt_level_mask &= (uint64_t) ~(1 << AVX2);
		}
	}

	FREE(avx2_skip_file);
	FREE(avx512_skip_file);

	return opt_level_mask;
}

bool use_file(uint64_t file_mask, uint64_t sys_mask)
{
	/* Order is very intentional for these tests */
	unsigned char file_options = file_mask & 255;
	unsigned char sys_options = sys_mask & 255;

	/* First SSE check is a slight performance optimization */
	/* If the file only supports SSE, just use it */
	if (file_options == SSE) {
		return true;
	}

	/* This must be done seperately as SSE == 0 and so the mask tests
	   after this won't detect SSE files matching */
	/* If the system and file only have SSE in common, use SSE */
	if ((sys_options & file_options) == SSE) {
		if ((file_mask >> 8) == SSE) {
			return true;
		} else {
			return false;
		}
	}

	/* Another check that is a slight performance optimization by
	   checking a special case to return early.
	   The SSE matching earlier is important as it would have been
	   missed by this check. */
	/* If the file isn't supported by the system don't use it */
	if (!((file_mask >> 8) & sys_mask)) {
		return false;
	}

	/* Look for the file with the system's highest supported optimization level */
	for (int i = MAX_OPTIMIZED_BIT_POWER; i >= 0; i--) {
		/* cur will eventually be non-zero as a previous check ensures that
		   the condition of sys_options and file_options without any bits
		   matching has already been handled */
		unsigned char cur = (sys_options & (1 << i)) & (file_options & (1 << i));
		if (cur) {
			/* best match between sys and file, use it if this file has the
			   current optimization level */
			if (cur & (file_mask >> 8)) {
				return true;
			} else {
				return false;
			}
		}
	}

	/* It shouldn't be possible to get here but don't do evil just in case */
	error("File optimization match failure\n");
	return (sys_options & (file_mask >> 8)) != 0;
}

struct manifest *manifest_parse(const char *component, const char *filename, bool header_only)
{
	FILE *infile;
	char line[MANIFEST_LINE_MAXLEN], *c, *c2;
	int count = 0;
	int deleted = 0;
	int err;
	struct manifest *manifest;
	struct list *includes = NULL;
	struct list *optional = NULL;
	unsigned long long filecount = 0;
	unsigned long long contentsize = 0;

	infile = fopen(filename, "rbm");
	if (infile == NULL) {
		return NULL;
	}

	manifest = malloc_or_die(sizeof(struct manifest));

	/* line 1: MANIFEST\t<version> */
	line[0] = 0;
	if (fgets(line, MANIFEST_LINE_MAXLEN, infile) == NULL) {
		goto err_close;
	}

	if (str_starts_with(line, MANIFEST_HEADER) != 0) {
		goto err_close;
	}

	c = line + str_len(MANIFEST_HEADER);
	err = str_to_int(c, &manifest->manifest_version);

	if (manifest->manifest_version <= 0 || err != 0) {
		error("Loaded incompatible manifest version\n");
		goto err_close;
	}

	line[0] = 0;
	while (str_starts_with(line, "\n") != 0) {
		/* read the header */
		line[0] = 0;
		if (fgets(line, MANIFEST_LINE_MAXLEN, infile) == NULL) {
			break;
		}

		// Remove line break
		c = strchr(line, '\n');
		if (!c) {
			goto err_close;
		}
		*c = 0;

		if (c == line) {
			break;
		}

		// Look for separator
		c = strchr(line, '\t');
		if (c) {
			c++;
		} else {
			goto err_close;
		}

		if (str_starts_with(line, "version:\t") == 0) {
			err = str_to_int(c, &manifest->version);
			if (err != 0) {
				error("Invalid manifest version on %s\n", filename);
				goto err_close;
			}
		} else if (str_starts_with(line, "filecount:\t") == 0) {
			errno = 0;
			filecount = strtoull(c, NULL, 10);
			if (filecount > 4000000) {
				/* Note on the number 4,000,000. We want this
				 * to be big enough to allow Manifest.Full to
				 * pass (currently about 450,000 March 18) but
				 * small enough that when multiplied by
				 * sizeof(struct file) it fits into
				 * size_t. For a system with size_t being a 32
				 * bit value, this constrains it to be less
				 * than about 6,000,000, but close to infinity
				 * for systems with 64 bit size_t.
				 */
				error("Preposterous (%llu) number of files in %s Manifest, more than 4 million skipping\n",
				      filecount, component);
				goto err_close;
			} else if (errno != 0) {
				error("Loaded incompatible manifest filecount\n");
				goto err_close;
			}
		} else if (str_starts_with(line, "contentsize:\t") == 0) {
			errno = 0;
			contentsize = strtoull(c, NULL, 10);
			if (contentsize > 2000000000000UL) {
				error("Preposterous (%llu) size of files in %s Manifest, more than 2TB skipping\n",
				      contentsize, component);
				goto err_close;
			} else if (errno != 0) {
				error("Loaded incompatible manifest contentsize\n");
				goto err_close;
			}
		} else if (str_starts_with(line, "includes:\t") == 0) {
			includes = list_prepend_data(includes, strdup_or_die(c));
		} else if (str_starts_with(line, "also-add:\t") == 0) {
			optional = list_prepend_data(optional, strdup_or_die(c));
		}
	}

	manifest->component = strdup_or_die(component);
	manifest->filecount = filecount;
	manifest->contentsize = contentsize;
	manifest->includes = includes;
	manifest->optional = optional;

	if (header_only) {
		fclose(infile);
		return manifest;
	}

	/* empty line */
	while (!feof(infile)) {
		struct file *file;
		uint64_t bitmask_translation;
		uint64_t file_mask;

		if (fgets(line, MANIFEST_LINE_MAXLEN, infile) == NULL) {
			break;
		}

		// Remove line break
		c = strchr(line, '\n');
		if (!c || c == line) {
			goto err_close;
		}
		*c = 0;

		// Look for separator
		c = strchr(line, '\t');
		if (c) {
			c++;
		} else {
			goto err_close;
		}

		file = malloc_or_die(sizeof(struct file));

		if (line[0] == 'F') {
			file->is_file = 1;
		} else if (line[0] == 'D') {
			file->is_dir = 1;
		} else if (line[0] == 'L') {
			file->is_link = 1;
		} else if (line[0] == 'M') {
			file->is_manifest = 1;
		}

		if (line[1] == 'd') {
			file->is_deleted = 1;
			deleted++;
		} else if (line[1] == 'g') {
			file->is_deleted = 1;
			file->is_ghosted = 1;
			deleted++;
		} else if (line[1] == 'e') {
			file->is_experimental = 1;
		}

		bitmask_translation = OPTIMIZED_BITMASKS[(unsigned char)line[2]];
		if (bitmask_translation > APX_7) {
			error("Skipping unsupported file optimization level\n");
			FREE(file);
			continue;
		}
		file_mask = LOOKUP_OPTIMIZED_BITMASKS[bitmask_translation];
		if (str_cmp("/", globals.path_prefix) != 0) {
			/* Don't use optimized binaries when prefix is set */
			/* as the operation might not be for the host system */
			/* so default to SSE for this case. */
			globals.opt_level_mask = (SSE << 8) | SSE;
		}
		if (!use_file(file_mask, globals.opt_level_mask)) {
			FREE(file);
			continue;
		}
		file->opt_mask = file_mask;

		if (line[3] == 'r') {
			/* rename flag is ignored */
		} else if (line[3] == 'x') {
			file->is_exported = 1;
		}

		c2 = strchr(c, '\t');
		if (c2) {
			*c2 = 0;
			c2++;
		} else {
			FREE(file);
			goto err_close;
		}

		hash_assign(c, file->hash);

		c = c2;
		c2 = strchr(c, '\t');
		if (c2) {
			*c2 = 0;
			c2++;
		} else {
			FREE(file);
			goto err_close;
		}

		err = str_to_int(c, &file->last_change);
		if (file->last_change <= 0 || err != 0) {
			error("Loaded incompatible manifest last change\n");
			FREE(file);
			goto err_close;
		}

		c = c2;

		file->filename = strdup_or_die(c);

		if (file->is_manifest) {
			manifest->manifests = list_prepend_data(manifest->manifests, file);
		} else {
			file->is_tracked = 1;
			manifest->files = list_prepend_data(manifest->files, file);
		}
		count++;
	}

	fclose(infile);
	return manifest;

err_close:
	manifest_free(manifest);
	fclose(infile);
	return NULL;
}

void manifest_free_data(void *data)
{
	struct manifest *manifest = (struct manifest *)data;

	manifest_free(manifest);
}

void manifest_free(struct manifest *manifest)
{
	if (!manifest) {
		return;
	}

	if (manifest->manifests) {
		list_free_list_and_data(manifest->manifests, free_file_data);
	}
	if (manifest->submanifests) {
		list_free_list(manifest->files);
		list_free_list_and_data(manifest->submanifests, manifest_free_data);
	} else {
		list_free_list_and_data(manifest->files, free_file_data);
	}
	if (manifest->includes) {
		list_free_list_and_data(manifest->includes, free);
	}
	if (manifest->optional) {
		list_free_list_and_data(manifest->optional, free);
	}
	FREE(manifest->component);
	FREE(manifest);
}
