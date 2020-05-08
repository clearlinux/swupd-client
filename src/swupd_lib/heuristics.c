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
	{"/boot/", h_starts_with, apply_boot },
	{"/usr/lib/modules/", h_starts_with, apply_boot },

	// State files
	{"/data", h_starts_with, apply_state },
	{"/dev/", h_starts_with, apply_state },
	{"/home/", h_starts_with, apply_state },
	{"/lost+found", h_starts_with, apply_state },
	{"/proc/", h_starts_with, apply_state },
	{"/root/", h_starts_with, apply_state },
	{"/run/", h_starts_with, apply_state },
	{"/sys/", h_starts_with, apply_state },
	{"/tmp/", h_starts_with, apply_state },
	{"/var/", h_starts_with, apply_state },

	// Filtered state on /usr/src
	{"/usr/src/", h_starts_with, apply_src_state },

	// Config files
	{"/etc/", h_starts_with, apply_config },

	// Boot managers
	{"/usr/bin/bootctl", h_strcmp, apply_bootmanager },
	{"/usr/bin/clr-boot-manager", h_strcmp, apply_bootmanager },
	{"/usr/bin/gummiboot", h_strcmp, apply_bootmanager },
	{"/usr/lib/gummiboot", h_strcmp, apply_bootmanager },
	{"/usr/share/syslinux/ldlinux.c32", h_strcmp, apply_bootmanager },

	{"/usr/lib/kernel/", h_starts_with, apply_boot_and_bootmanager },
	{"/usr/lib/systemd/boot", h_starts_with, apply_boot_and_bootmanager },

	// Systemd
	{"/usr/lib/systemd/systemd", h_strcmp, apply_systemd},

	{ 0 }
};

static bool check_in_mounted_directory(char *filename)
{
	if (is_directory_mounted(filename)) {
		return true;
	}

	return false;
}

static void runtime_state_heuristics(struct file *file)
{
	if (check_in_mounted_directory(file->filename)) {
		file->is_state = 1;
	}
}

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

static void apply_heuristics_for_file(struct list *rules, struct file *file)
{
	struct list *iter;

	// Apply all rules
	for (iter = rules; iter; iter = iter->next) {
		const struct rule *rule = iter->data;
		if (rule->cmp(file->filename, rule->str) == 0) {
			rule->apply(file);
		}
	}

	runtime_state_heuristics(file);
	check_ignore_file(file);
}

void heuristics_apply(struct list *files)
{
	struct list *rules = NULL;
	struct list *iter;
	struct file *file;
	int i;

	// Create the rules list
	for (i = 0; heuristic_rules[i].str; i++) {
		rules = list_prepend_data(rules, dup_rule(&heuristic_rules[i]));
	}

	// Apply the rules
	for (iter = files; iter; iter = iter->next) {
		file = iter->data;
		apply_heuristics_for_file(rules, file);
	}

	list_free_list_and_data(rules, free);
}
