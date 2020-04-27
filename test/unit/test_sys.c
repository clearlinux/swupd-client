#include <stdio.h>

#include "../../src/lib/sys.h"
#include "../../src/lib/strings.h"
#include "test_helper.h"

static void test_sys_path_join()
{
	char *path;
	size_t i;

	path = sys_path_join("%s/%s/%s", "/123", "456", "789");
	check(str_cmp(path, "/123/456/789") == 0);
	free(path);

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
		free(path);
	}
}

int main() {
	test_sys_path_join();

	return 0;
}
