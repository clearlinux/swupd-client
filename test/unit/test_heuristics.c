/*
 *   Software Updater - client side
 *
 *      Copyright (c) 2019-2020 Intel Corporation.
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
 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "test_helper.h"
#include "lib/list.h"
#include "swupd.h"
#include "swupd_lib/globals.h"
#include "swupd_lib/heuristics.h"

static struct file *new_file(char *filename)
{
	struct file *f = malloc_or_die(sizeof(struct file));

	f->filename = filename;

	return f;
}

void test_heuristics()
{
	struct hfile {
		char *filename;
		bool is_boot;
		bool is_config;
		bool is_state;
		bool do_not_update;
		bool need_systemd_reexec;
		bool need_update_bootmanager;
	};

	struct hfile test_files[] =
	{
	{"/",                          false, false, false, false, false, false},
	{"/usr",                       false, false, false, false, false, false},
	{"/usr/bin/",                  false, false, false, false, false, false},
	{"/usr/bin/ls",                false, false, false, false, false, false},
	{"/usr/lib/my_lib.so",         false, false, false, false, false, false},

	// Only set is_boot
	{"/boot/file1",                true, false, false, false, false, false},
	{"/usr/lib/modules/file",      true, false, false, false, false, false},

	// Only update bootmanager
	{"/usr/bin/clr-boot-manager",  false, false, false, false, false, true},
	{"/usr/lib/gummiboot",         false, false, false, false, false, true},
	{"/usr/bin/gummiboot",	       false, false, false, false, false, true},
	{"/usr/bin/bootctl",           false, false, false, false, false, true},
	{"/usr/bin/clr-boot-manager",  false, false, false, false, false, true},
	{"/usr/share/syslinux/ldlinux.c32", false, false, false, false, false, true},

	// update bootmanager and is_boot
	{"/usr/lib/kernel/my_kernel",  true, false, false, false, false, true},
	{"/usr/lib/systemd/boot",      true, false, false, false, false, true},

	// Update globals.need_systemd_reexec
	{"/usr/lib/systemd/systemd",  false, false, false, false, true, false},

	// do_not_update
	{"/etc/file",                 false, true, false, true, false, false},
	{"/usr/src/debug/file",       false, false, true, true, false, false},
	{"/usr/src/anything_else",    false, false, true, true, false, false},
	{"/data/file",                false, false, true, true, false, false},
	{"/dev/file",                 false, false, true, true, false, false},
	{"/home/file",                false, false, true, true, false, false},
	{"/lost+found/file",          false, false, true, true, false, false},
	{"/proc/file",                false, false, true, true, false, false},
	{"/root/file",                false, false, true, true, false, false},
	{"/run/file",                 false, false, true, true, false, false},
	{"/sys/file",                 false, false, true, true, false, false},
	{"/tmp/file",                 false, false, true, true, false, false},
	{"/var/file",                 false, false, true, true, false, false},
	{"/usr/src/kernel123",        false, false, true, true, false, false},
	{"/usr/src/debug123",         false, false, true, true, false, false},
	{"/usr/src/debug/files",      false, false, true, true, false, false},

	// do_not_update == false
	{"/usr/src/debug",            false, false, false, false, false, false},
	{"/usr/src/kernel",           false, false, false, false, false, false},
	{"/usr/src/kernel/file",      false, false, false, false, false, false},

	{ 0 }
	};

	int i;

	// Test heuristics on files one by one
	for (i = 0; test_files[i].filename; i++) {
		struct list *files;
		struct file *f;

		f = new_file(test_files[i].filename);
		files = list_prepend_data(NULL, f);
		globals.need_update_bootmanager = false;
		globals.need_systemd_reexec = false;
		heuristics_apply(files);

		check(f->is_boot == test_files[i].is_boot);
		check(f->is_state == test_files[i].is_state);
		check(f->is_config == test_files[i].is_config);
		check(f->do_not_update == test_files[i].do_not_update);

		check(globals.need_systemd_reexec == test_files[i].need_systemd_reexec);
		check(globals.need_update_bootmanager == test_files[i].need_update_bootmanager);

		list_free_list_and_data(files, free);
	}


	// Test if no heuristic is ignore when running on a list of all files
	struct list *files = NULL;
	for (i = 0; test_files[i].filename; i++) {
		struct file *f;

		f = new_file(strdup_or_die(test_files[i].filename));
		files = list_prepend_data(files, f);
	}
	files = list_sort(files, cmp_file_filename_is_deleted);
	heuristics_apply(files);
	for (i = 0; test_files[i].filename; i++) {
		struct list *iter;
		for (iter = files; iter; iter = iter->next) {
			struct file *f = iter->data;
			if (str_cmp(f->filename, test_files[i].filename) == 0) {
				check(f->is_boot == test_files[i].is_boot);
				check(f->is_state == test_files[i].is_state);
				check(f->is_config == test_files[i].is_config);
				check(f->do_not_update == test_files[i].do_not_update);
				break;
			}
		}
	}
	list_free_list_and_data(files, free_file_data);
}

int main() {
	test_heuristics();

	return 0;
}
