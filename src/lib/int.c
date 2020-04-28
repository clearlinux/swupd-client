#include "int.h"
#include "log.h"
#include "macros.h"

#include <errno.h>
#include <limits.h>

int long_to_ulong_err(long a, unsigned long *b)
{
	if (!b) {
		UNEXPECTED();
		debug("Invalid parameter: b is NULL\n");
		return -EINVAL;
	}

	if (a < 0) {
		return -ERANGE;
	}

	*b = (unsigned long)a;
	return 0;
}

unsigned long long_to_ulong(long a)
{
	unsigned long b;
	if (long_to_ulong_err(a, &b) == 0) {
		return b;
	}

	return 0;
}

int int_to_uint_err(int a, unsigned int *b)
{
	if (!b) {
		UNEXPECTED();
		debug("Invalid parameter: b is NULL\n");
		return -EINVAL;
	}

	if (a < 0) {
		return -ERANGE;
	}

	*b = (unsigned int)a;
	return 0;
}

unsigned int int_to_uint(int a)
{
	unsigned int b;
	if (int_to_uint_err(a, &b) == 0) {
		return b;
	}

	return 0;
}

int uint_to_int_err(unsigned int a, int *b)
{
	if (!b) {
		UNEXPECTED();
		debug("Invalid parameter: b is NULL\n");
		return -EINVAL;
	}

	if (a > INT_MAX) {
		return -ERANGE;
	}

	*b = (int)a;
	return 0;
}

int uint_to_int(unsigned int a)
{
	int b;
	if (uint_to_int_err(a, &b) == 0) {
		return b;
	}

	return INT_MAX;
}

int ulong_to_long_err(unsigned long a, long *b)
{
	if (!b) {
		UNEXPECTED();
		debug("Invalid parameter: b is NULL\n");
		return -EINVAL;
	}

	if (a > LONG_MAX) {
		return -ERANGE;
	}

	*b = (long)a;
	return 0;
}

long ulong_to_long(unsigned long a)
{
	long b;
	if (ulong_to_long_err(a, &b) == 0) {
		return b;
	}

	return LONG_MAX;
}

int ulong_to_int_err(unsigned long a, int *b)
{
	if (!b) {
		UNEXPECTED();
		debug("Invalid parameter: b is NULL\n");
		return -EINVAL;
	}

	if (a > INT_MAX) {
		return -ERANGE;
	}

	*b = (int)a;
	return 0;
}

int ulong_to_int(unsigned long a)
{
	int b;
	if (ulong_to_int_err(a, &b) == 0) {
		return b;
	}

	return INT_MAX;
}

int long_to_int_err(long a, int *b)
{
	if (!b) {
		UNEXPECTED();
		debug("Invalid parameter: b is NULL\n");
		return -EINVAL;
	}

	if (a > INT_MAX || a < INT_MIN) {
		return -ERANGE;
	}

	*b = (int)a;
	return 0;
}

int long_to_int(long a)
{
	int b;
	if (long_to_int_err(a, &b) == 0) {
		return b;
	}

	if (a < 0) {
		return INT_MIN;
	}

	return INT_MAX;
}
