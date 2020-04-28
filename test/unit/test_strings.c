#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "../../src/lib/strings.h"
#include "../../src/lib/list.h"
#include "../../src/lib/macros.h"
#include "test_helper.h"

static void test_str_to_int()
{
	char tmp_str[100];
	int err;
	int value = 0;
	size_t i;

	// test some standard values
	char *str_values[] = { "0", "-0", "123", "000123", "-123" };
	int values[] = { 0, 0, 123, 123, -123 };

	for (i = 0; i < sizeof(values) / sizeof(int); i++) {
		value = 0xffffff;
		err = str_to_int(str_values[i], &value);
		check(err == 0);
		check(value == values[i]);
	}

	// test limits
	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%d", INT_MAX - 1);
	err = str_to_int(tmp_str, &value);
	check(err == 0);
	check(value == INT_MAX - 1);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%d", INT_MAX);
	err = str_to_int(tmp_str, &value);
	check(err == 0);
	check(value == INT_MAX);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%d", INT_MIN + 1);
	err = str_to_int(tmp_str, &value);
	check(err == 0);
	check(value == INT_MIN + 1);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%d", INT_MIN);
	err = str_to_int(tmp_str, &value);
	check(err == 0);
	check(value == INT_MIN);

	// test errors
	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%ld", (long)INT_MAX + 1);
	err = str_to_int(tmp_str, &value);
	check(err == -ERANGE);
	check(value == INT_MAX);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%ld", (long)INT_MAX + 1000);
	err = str_to_int(tmp_str, &value);
	check(err == -ERANGE);
	check(value == INT_MAX);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%ld", (long)INT_MIN - 1);
	err = str_to_int(tmp_str, &value);
	check(err == -ERANGE);
	check(value == INT_MIN);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%ld", (long)INT_MIN - 1000);
	err = str_to_int(tmp_str, &value);
	check(err == -ERANGE);
	check(value == INT_MIN);

	// test EINVAL errors
	char *einval_errors[] = { "string", "123.456", "123a", "a123a", "456b123", "0x12345", "4f"};

	for (i = 0; i < sizeof(einval_errors) / sizeof(char *); i++) {
		err = str_to_int(einval_errors[i], &value);
		check(err == -EINVAL);
	}
}

static void test_str_to_int_endptr()
{
	char tmp_str[100];
	char *endptr;
	int err;
	int value = 0;
	size_t i;

	// test some standard values
	char *str_values[] = { "0", "-0", "123", "000123", "-123" };
	int values[] = { 0, 0, 123, 123, -123 };

	for (i = 0; i < sizeof(values) / sizeof(int); i++) {
		value = 0xffffff;
		err = str_to_int_endptr(str_values[i], NULL, &value);
		check(err == 0);
		check(value == values[i]);
	}

	// test limits
	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%d", INT_MAX - 1);
	err = str_to_int_endptr(tmp_str, NULL, &value);
	check(err == 0);
	check(value == INT_MAX - 1);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%d", INT_MAX);
	err = str_to_int_endptr(tmp_str, NULL, &value);
	check(err == 0);
	check(value == INT_MAX);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%d", INT_MIN + 1);
	err = str_to_int_endptr(tmp_str, NULL, &value);
	check(err == 0);
	check(value == INT_MIN + 1);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%d", INT_MIN);
	err = str_to_int_endptr(tmp_str, NULL, &value);
	check(err == 0);
	check(value == INT_MIN);

	// test errors
	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%ld", (long)INT_MAX + 1);
	err = str_to_int_endptr(tmp_str, NULL, &value);
	check(err == -ERANGE);
	check(value == INT_MAX);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%ld", (long)INT_MAX + 1000);
	err = str_to_int_endptr(tmp_str, NULL, &value);
	check(err == -ERANGE);
	check(value == INT_MAX);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%ld", (long)INT_MIN - 1);
	err = str_to_int_endptr(tmp_str, NULL, &value);
	check(err == -ERANGE);
	check(value == INT_MIN);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%ld", (long)INT_MIN - 1000);
	err = str_to_int_endptr(tmp_str, NULL, &value);
	check(err == -ERANGE);
	check(value == INT_MIN);

	// Test endptr trailing chars
	value = 0xffffff;
	endptr = NULL;
	err = str_to_int_endptr("123.456", &endptr, &value);
	check(err == 0);
	check(value == 123);
	check(endptr != NULL);
	check(*endptr == '.');

	value = 0xffffff;
	endptr = NULL;
	err = str_to_int_endptr("123a456", &endptr, &value);
	check(err == 0);
	check(value == 123);
	check(endptr != NULL);
	check(*endptr == 'a');

	value = 0xffffff;
	endptr = NULL;
	err = str_to_int_endptr("123testing456", &endptr, &value);
	check(err == 0);
	check(value == 123);
	check(endptr != NULL);
	check(*endptr == 't');

	value = 0xffffff;
	endptr = NULL;
	err = str_to_int_endptr("123-456", &endptr, &value);
	check(err == 0);
	check(value == 123);
	check(endptr != NULL);
	check(*endptr == '-');

	value = 0xffffff;
	endptr = NULL;
	err = str_to_int_endptr("-123.456", &endptr, &value);
	check(err == 0);
	check(value == -123);
	check(endptr != NULL);
	check(*endptr == '.');

	value = 0xffffff;
	endptr = NULL;
	err = str_to_int_endptr("-123a456", &endptr, &value);
	check(err == 0);
	check(value == -123);
	check(endptr != NULL);
	check(*endptr == 'a');

	value = 0xffffff;
	endptr = NULL;
	err = str_to_int_endptr("-123testing456", &endptr, &value);
	check(err == 0);
	check(value == -123);
	check(endptr != NULL);
	check(*endptr == 't');

	value = 0xffffff;
	endptr = NULL;
	err = str_to_int_endptr("-123-456", &endptr, &value);
	check(err == 0);
	check(value == -123);
	check(endptr != NULL);
	check(*endptr == '-');

}

static void test_str_to_uint()
{
	char tmp_str[100];
	int err;
	unsigned int value = 0;
	size_t i;

	// test some standard values
	char *str_values[] = { "0", "-0", "123", "000123" };
	unsigned int values[] = { 0, 0, 123, 123 };

	for (i = 0; i < sizeof(values) / sizeof(int); i++) {
		value = 0xffffff;
		err = str_to_uint(str_values[i], &value);
		check(err == 0);
		check(value == values[i]);
	}

	// test limits
	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%u", UINT_MAX - 1);
	err = str_to_uint(tmp_str, &value);
	check(err == 0);
	check(value == UINT_MAX - 1);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%u", UINT_MAX);
	err = str_to_uint(tmp_str, &value);
	check(err == 0);
	check(value == UINT_MAX);

	// test errors
	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%ld", (long)UINT_MAX + 1);
	err = str_to_uint(tmp_str, &value);
	check(err == -ERANGE);
	check(value == UINT_MAX);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%ld", (long)UINT_MAX + 1000);
	err = str_to_uint(tmp_str, &value);
	check(err == -ERANGE);
	check(value == UINT_MAX);

	// Negative
	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%d", -1);
	err = str_to_uint(tmp_str, &value);
	check(err == -ERANGE);
	check(value == 0);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%d", INT_MIN);
	err = str_to_uint(tmp_str, &value);
	check(err == -ERANGE);
	check(value == 0);

	// test EINVAL errors
	char *einval_errors[] = { "string", "123.456", "123a", "a123a", "456b123", "0x12345", "4f"};

	for (i = 0; i < sizeof(einval_errors) / sizeof(char *); i++) {
		err = str_to_uint(einval_errors[i], &value);
		check(err == -EINVAL);
	}
}

static void test_str_to_uint_endptr()
{
	char *endptr;
	int err;
	unsigned int value = 0;

	// Test endptr trailing chars
	value = 0xffffff;
	endptr = NULL;
	err = str_to_uint_endptr("123.456", &endptr, &value);
	check(err == 0);
	check(value == 123);
	check(endptr != NULL);
	check(*endptr == '.');

	value = 0xffffff;
	endptr = NULL;
	err = str_to_uint_endptr("123a456", &endptr, &value);
	check(err == 0);
	check(value == 123);
	check(endptr != NULL);
	check(*endptr == 'a');

	value = 0xffffff;
	endptr = NULL;
	err = str_to_uint_endptr("123testing456", &endptr, &value);
	check(err == 0);
	check(value == 123);
	check(endptr != NULL);
	check(*endptr == 't');

	value = 0xffffff;
	endptr = NULL;
	err = str_to_uint_endptr("123-456", &endptr, &value);
	check(err == 0);
	check(value == 123);
	check(endptr != NULL);
	check(*endptr == '-');

	value = 0xffffff;
	endptr = NULL;
	err = str_to_uint_endptr("-123.456", &endptr, &value);
	check(err == -ERANGE);

	value = 0xffffff;
	endptr = NULL;
	err = str_to_uint_endptr("-123a456", &endptr, &value);
	check(err == -ERANGE);

}

void test_str_join()
{
	struct list *str_list = NULL;
	char *joined;

	joined = str_join(" long separator ", str_list);
	check(str_cmp(joined, "") == 0);
	FREE(joined);

	str_list = list_prepend_data(str_list, "string3");

	joined = str_join(" long separator ", str_list);
	check(str_cmp(joined, "string3") == 0);
	FREE(joined);

	str_list = list_prepend_data(str_list, "string2");
	str_list = list_prepend_data(str_list, "string1");

	joined = str_join(",", str_list);
	check(str_cmp(joined, "string1,string2,string3") == 0);
	FREE(joined);

	joined = str_join(" long separator ", str_list);
	check(str_cmp(joined, "string1 long separator string2 long separator string3") == 0);
	FREE(joined);

	joined = str_join("", str_list);
	check(str_cmp(joined, "string1string2string3") == 0);
	FREE(joined);

	joined = str_join(NULL, str_list);
	check(str_cmp(joined, "string1string2string3") == 0);
	FREE(joined);

	list_free_list(str_list);
}

int main() {
	test_str_to_int();
	test_str_to_int_endptr();
	test_str_join();
	test_str_to_uint();
	test_str_to_uint_endptr();

	return 0;
}
