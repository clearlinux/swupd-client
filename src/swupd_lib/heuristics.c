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
 *   Authors:
 *         Timothy C. Pepper <timothy.c.pepper@linux.intel.com>
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "swupd.h"
#include "heuristics.h"

typedef int (*compare_fn_t)(const char *s1, const char *s2);
typedef void (*apply_fn_t)(struct file *f);

struct rule {
	char *str;
	compare_fn_t cmp;
	apply_fn_t apply;
	bool is_mounted;
};

static void apply_config(struct file *f)
{
	f->is_config = 1;
}

static void apply_state(struct file *f)
{
	f->is_state = 1;
}

static void apply_boot(struct file *f)
{
	f->is_boot = 1;
}

static void apply_boot_and_bootmanager(struct file *f)
{
	f->is_boot = 1;
	globals.need_update_bootmanager = true;
}

static void apply_bootmanager(struct file UNUSED_PARAM *f)
{
	globals.need_update_bootmanager = true;
}

static void apply_systemd(struct file UNUSED_PARAM *f)
{
	globals.need_systemd_reexec = true;
}

static void apply_src_state(struct file *f)
{
	// /usr/src/debug, /usr/src/kernel and everything inside /usr/src/kernel/
	// directory is not a state
	if (str_cmp(f->filename, "/usr/src/debug") == 0 ||
	    str_cmp(f->filename, "/usr/src/kernel") == 0 ||
	    str_starts_with(f->filename, "/usr/src/kernel/") == 0) {
		return;
	}

	f->is_state = 1;
}

static int h_starts_with(const char *s1, const char *s2)
{
	return str_starts_with(s1, s2);
}

static int h_strcmp(const char *s1, const char *s2)
{
	return str_cmp(s1, s2);
}

static const struct rule heuristic_rules[] = {
	// Boot Files
	{"/boot/", h_starts_with, apply_boot, false },
	{"/usr/lib/modules/", h_starts_with, apply_boot, false },

	// State files
	{"/data", h_starts_with, apply_state, false },
	{"/dev/", h_starts_with, apply_state, false },
	{"/home/", h_starts_with, apply_state, false },
	{"/lost+found", h_starts_with, apply_state, false },
	{"/proc/", h_starts_with, apply_state, false },
	{"/root/", h_starts_with, apply_state, false },
	{"/run/", h_starts_with, apply_state, false },
	{"/sys/", h_starts_with, apply_state, false },
	{"/tmp/", h_starts_with, apply_state, false },
	{"/var/", h_starts_with, apply_state, false },

	// Filtered state on /usr/src
	{"/usr/src/", h_starts_with, apply_src_state, false },

	// Config files
	{"/etc/", h_starts_with, apply_config, false },

	// Boot managers
	{"/usr/bin/bootctl", h_strcmp, apply_bootmanager, false },
	{"/usr/bin/clr-boot-manager", h_strcmp, apply_bootmanager, false },
	{"/usr/bin/gummiboot", h_strcmp, apply_bootmanager, false },
	{"/usr/lib/gummiboot", h_strcmp, apply_bootmanager, false },
	{"/usr/share/syslinux/ldlinux.c32", h_strcmp, apply_bootmanager, false },

	{"/usr/lib/kernel/", h_starts_with, apply_boot_and_bootmanager, false },
	{"/usr/lib/systemd/boot", h_starts_with, apply_boot_and_bootmanager, false },

	// Systemd
	{"/usr/lib/systemd/systemd", h_strcmp, apply_systemd, false},

	{ 0 }
};

/* Determines whether or not FILE should be ignored for this swupd action. Note
 * that boot files are ignored only if they are marked as deleted; this does
 * not happen in current manifests produced by swupd-server, but this check is
 * enabled in case swupd-server ever allows for deleted boot files in manifests
 * in the future.
 */
static void check_ignore_file(struct file *file)
{
	if ((OS_IS_STATELESS && file->is_config) ||
	    (file->is_state) ||
	    (file->is_boot && file->is_deleted) ||
	    (file->is_orphan) ||
	    (file->is_ghosted)) {
		file->do_not_update = 1;
	}
}

static struct rule *dup_rule(const struct rule *r)
{
	struct rule * rule;

	rule = malloc_or_die(sizeof(struct rule));

	*rule = *r;

	return rule;
}

static struct rule *create_rule(char *str, compare_fn_t cmp, apply_fn_t apply)
{
	struct rule * rule;

	rule = malloc_or_die(sizeof(struct rule));

	rule->str = str;
	rule->cmp = cmp;
	rule->apply = apply;
	rule->is_mounted = true;

	return rule;
}

static struct list *create_rules_from_mounted_dirs(void)
{
	struct list *rules = NULL;
	struct list *mounted_dirs = NULL;
	char *path_prefix, *filename;
	size_t path_prefix_len;
	struct list *iter;

	path_prefix = sys_path_append_separator(globals.path_prefix);
	path_prefix_len = str_len(path_prefix);
	mounted_dirs = sys_get_mounted_directories();

	// Only create rules for mounted directories inside path_prefix
	// And rule should be relative to the path_prefix we are working on
	for (iter = mounted_dirs; iter; iter = iter->next) {
		if (str_starts_with(iter->data, path_prefix) == 0) {
			// Get the filename of the directory inside the chroot
			// sed s/^path_prefix/\//
			filename = sys_path_join("/%s", iter->data + path_prefix_len);
			rules = list_prepend_data(rules,
				create_rule(filename, h_strcmp, apply_state));
		}
	}

	list_free_list_and_data(mounted_dirs, free);
	FREE(path_prefix);

	return rules;
}

static void free_rule(void *data)
{
	struct rule *r = data;

	if (r->is_mounted) {
		FREE(r->str);
	}
	FREE(r);
}

static int rule_cmp(const void *d1, const void *d2)
{
	int ret;
	const struct rule *r1 = d1;
	const struct rule *r2 = d2;

	ret = str_cmp(r1->str, r2->str);
	if (ret != 0) {
		return ret;
	}

	// Place is_mounted rules after static rules
	if (r1->is_mounted == r2->is_mounted) {
		return 0;
	}

	if (r2->is_mounted) {
		return -1;
	}

	return 1;
}

static void apply_rules_to_files(struct list *rules, struct list *files)
{
	// Loop all files
	while (files && rules) {
		struct file *file = files->data;
		const struct rule *r = rules->data;
		int comparison;

		comparison = r->cmp(file->filename, r->str);

		// If matches, apply rule
		if (comparison == 0) {
			r->apply(file);
		}

		if (comparison > 0) {
			// Move to next rule or next element in the list
			rules = rules->next;
		} else {
			// Move to next file
			files = files->next;
		}
	}
}

void heuristics_apply(struct list *files)
{
	struct list *rules = NULL;
	struct list *iter;
	int i;

	// Make sure list is sorted
	if (!list_is_sorted(files, cmp_file_filename_is_deleted)) {
		UNEXPECTED();
		debug("List of files to apply heuristics is not sorted - fixing\n");
		files = list_sort(files, cmp_file_filename_is_deleted);
	}

	// Apply static rules
	for (i = 0; heuristic_rules[i].str; i++) {
		rules = list_prepend_data(rules, dup_rule(&heuristic_rules[i]));
	}
	rules = list_sort(rules, rule_cmp);
	apply_rules_to_files(rules, files);
	list_free_list_and_data(rules, free_rule);

	// Apply mounted dirs rules
	rules = list_sort(create_rules_from_mounted_dirs(), rule_cmp);
	apply_rules_to_files(rules, files);
	list_free_list_and_data(rules, free_rule);

	// Apply ignore rules to all files
	for (iter = files; iter; iter = iter->next) {
		check_ignore_file(iter->data);
	}
}
