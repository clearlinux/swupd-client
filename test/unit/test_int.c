#include "../../src/lib/int.h"
#include "test_helper.h"

#include <limits.h>
#include <errno.h>

static void test_int_to_uint()
{
	int ret;
	unsigned int b;

	ret = int_to_uint_err(0, &b);
	check(ret == 0);
	check(b == 0);

	ret = int_to_uint_err(123, &b);
	check(ret == 0);
	check(b == 123);

	ret = int_to_uint_err(INT_MAX, &b);
	check(ret == 0);
	check(b == INT_MAX);

	ret = int_to_uint_err(-1, &b);
	check(ret == -ERANGE);

	ret = int_to_uint_err(-INT_MAX, &b);
	check(ret == -ERANGE);

	ret = int_to_uint_err(-INT_MAX - 1, &b);
	check(ret == -ERANGE);
}

static void test_ssize_to_size()
{
	int ret;
	size_t b;

	ret = ssize_to_size_err(0, &b);
	check(ret == 0);
	check(b == 0);

	ret = ssize_to_size_err(123, &b);
	check(ret == 0);
	check(b == 123);

	ret = ssize_to_size_err(SSIZE_MAX, &b);
	check(ret == 0);
	check(b == SSIZE_MAX);

	ret = ssize_to_size_err(-1, &b);
	check(ret == -ERANGE);

	ret = ssize_to_size_err(-SSIZE_MAX, &b);
	check(ret == -ERANGE);

	ret = ssize_to_size_err(-SSIZE_MAX - 1, &b);
	check(ret == -ERANGE);
}

int main()
{
	test_ssize_to_size();
	test_int_to_uint();

	return 0;
}
