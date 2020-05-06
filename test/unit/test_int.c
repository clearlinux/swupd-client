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

static void test_long_to_ulong()
{
	int ret;
	size_t b;

	ret = long_to_ulong_err(0, &b);
	check(ret == 0);
	check(b == 0);

	ret = long_to_ulong_err(123, &b);
	check(ret == 0);
	check(b == 123);

	ret = long_to_ulong_err(SSIZE_MAX, &b);
	check(ret == 0);
	check(b == SSIZE_MAX);

	ret = long_to_ulong_err(-1, &b);
	check(ret == -ERANGE);

	ret = long_to_ulong_err(-SSIZE_MAX, &b);
	check(ret == -ERANGE);

	ret = long_to_ulong_err(-SSIZE_MAX - 1, &b);
	check(ret == -ERANGE);
}

static void test_uint_to_int()
{
	int ret;
	int b;

	ret = uint_to_int_err(0, &b);
	check(ret == 0);
	check(b == 0);

	ret = uint_to_int_err(123, &b);
	check(ret == 0);
	check(b == 123);

	ret = uint_to_int_err(INT_MAX, &b);
	check(ret == 0);
	check(b == INT_MAX);

	ret = uint_to_int_err((unsigned int)INT_MAX + 1, &b);
	check(ret == -ERANGE);

	ret = uint_to_int_err(UINT_MAX, &b);
	check(ret == -ERANGE);

}

static void test_ulong_to_long()
{
	int ret;
	ssize_t b;

	ret = ulong_to_long_err(0, &b);
	check(ret == 0);
	check(b == 0);

	ret = ulong_to_long_err(123, &b);
	check(ret == 0);
	check(b == 123);

	ret = ulong_to_long_err(SSIZE_MAX, &b);
	check(ret == 0);
	check(b == SSIZE_MAX);

	ret = ulong_to_long_err((size_t)SSIZE_MAX + 1, &b);
	check(ret == -ERANGE);

	ret = ulong_to_long_err(SIZE_MAX, &b);
	check(ret == -ERANGE);
}

static void test_ulong_to_int()
{
	int ret;
	int b;

	ret = ulong_to_int_err(0, &b);
	check(ret == 0);
	check(b == 0);

	ret = ulong_to_int_err(123, &b);
	check(ret == 0);
	check(b == 123);

	ret = ulong_to_int_err(INT_MAX, &b);
	check(ret == 0);
	check(b == INT_MAX);

	ret = ulong_to_int_err((size_t)INT_MAX + 1, &b);
	check(ret == -ERANGE);

	ret = ulong_to_int_err(SIZE_MAX, &b);
	check(ret == -ERANGE);
}

int main()
{
	test_int_to_uint();
	test_long_to_ulong();
	test_uint_to_int();
	test_ulong_to_long();
	test_ulong_to_int();

	return 0;
}
