/*
 *   Software Updater - client side
 *
 *      Copyright (c) 2019-2025 Intel Corporation.
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

#include <stdio.h>

#include "../../src/lib/macros.h"
#include "../../src/lib/sys.h"
#include "../../src/lib/strings.h"
#include "test_helper.h"

static void test_sys_path_join()
{
	char *path;
	size_t i;

	path = sys_path_join("%s/%s/%s", "/123", "456", "789");
	check(str_cmp(path, "/123/456/789") == 0);
	FREE(path);

	const char *tests[][2] = {{"", ""},
				  {"/", "/"},
				  {"/usr/", "/usr"},
				  {"/usr//", "/usr"},
				  {"/usr///", "/usr"},
				  {"//usr", "/usr"},
				  {"///////usr///////", "/usr"},
				  {"//usr//lib//test//////", "/usr/lib/test"},
				  {"//////", "/"},
				  {"",""}};

	for (i = 0; i < sizeof(tests)/(sizeof(char *) * 2); i++) {
		path = sys_path_join(tests[i][0]);
		printf("test %s -> %s\n", tests[i][0], path);
		check(str_cmp(path, tests[i][1]) == 0);
		FREE(path);
	}
}

int main() {
	test_sys_path_join();

	return 0;
}
