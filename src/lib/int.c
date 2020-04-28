#include "int.h"
#include "log.h"
#include "macros.h"

#include <errno.h>

int ssize_to_size_err(ssize_t a, size_t *b)
{
	if (!b) {
		UNEXPECTED();
		debug("Invalid parameter: b is NULL");
		return -EINVAL;
	}

	if (a < 0) {
		return -ERANGE;
	}

	*b = (size_t)a;
	return 0;
}

int int_to_uint_err(int a, unsigned int *b)
{
	if (!b) {
		UNEXPECTED();
		debug("Invalid parameter: b is NULL");
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

size_t ssize_to_size(int a)
{
	size_t b;
	if (ssize_to_size_err(a, &b) == 0) {
		return b;
	}

	return 0;
}
