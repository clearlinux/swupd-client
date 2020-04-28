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

static void test_size_to_ssize()
{
	int ret;
	ssize_t b;

	ret = size_to_ssize_err(0, &b);
	check(ret == 0);
	check(b == 0);

	ret = size_to_ssize_err(123, &b);
	check(ret == 0);
	check(b == 123);

	ret = size_to_ssize_err(SSIZE_MAX, &b);
	check(ret == 0);
	check(b == SSIZE_MAX);

	ret = size_to_ssize_err((size_t)SSIZE_MAX + 1, &b);
	check(ret == -ERANGE);

	ret = size_to_ssize_err(SIZE_MAX, &b);
	check(ret == -ERANGE);
}

static void test_size_to_int()
{
	int ret;
	int b;

	ret = size_to_int_err(0, &b);
	check(ret == 0);
	check(b == 0);

	ret = size_to_int_err(123, &b);
	check(ret == 0);
	check(b == 123);

	ret = size_to_int_err(INT_MAX, &b);
	check(ret == 0);
	check(b == INT_MAX);

	ret = size_to_int_err((size_t)INT_MAX + 1, &b);
	check(ret == -ERANGE);

	ret = size_to_int_err(SIZE_MAX, &b);
	check(ret == -ERANGE);
}

int main()
{
	test_int_to_uint();
	test_ssize_to_size();
	test_uint_to_int();
	test_size_to_ssize();
	test_size_to_int();

	return 0;
}
