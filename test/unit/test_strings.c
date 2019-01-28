#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "../../src/lib/strings.h"
#include "../../src/lib/list.h"
#include "test_helper.h"

static void test_strtoi_err()
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
		err = strtoi_err(str_values[i], &value);
		check(err == 0);
		check(value == values[i]);
	}

	// test limits
	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%d", INT_MAX - 1);
	err = strtoi_err(tmp_str, &value);
	check(err == 0);
	check(value == INT_MAX - 1);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%d", INT_MAX);
	err = strtoi_err(tmp_str, &value);
	check(err == 0);
	check(value == INT_MAX);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%d", INT_MIN + 1);
	err = strtoi_err(tmp_str, &value);
	check(err == 0);
	check(value == INT_MIN + 1);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%d", INT_MIN);
	err = strtoi_err(tmp_str, &value);
	check(err == 0);
	check(value == INT_MIN);

	// test errors
	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%ld", (long)INT_MAX + 1);
	err = strtoi_err(tmp_str, &value);
	check(err == -ERANGE);
	check(value == INT_MAX);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%ld", (long)INT_MAX + 1000);
	err = strtoi_err(tmp_str, &value);
	check(err == -ERANGE);
	check(value == INT_MAX);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%ld", (long)INT_MIN - 1);
	err = strtoi_err(tmp_str, &value);
	check(err == -ERANGE);
	check(value == INT_MIN);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%ld", (long)INT_MIN - 1000);
	err = strtoi_err(tmp_str, &value);
	check(err == -ERANGE);
	check(value == INT_MIN);

	// test EINVAL errors
	char *einval_errors[] = { "string", "123.456", "123a", "a123a", "456b123", "0x12345", "4f"};

	for (i = 0; i < sizeof(einval_errors) / sizeof(char *); i++) {
		err = strtoi_err(einval_errors[i], &value);
		check(err == -EINVAL);
	}
}

static void test_strtoi_err_endptr()
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
		err = strtoi_err_endptr(str_values[i], NULL, &value);
		check(err == 0);
		check(value == values[i]);
	}

	// test limits
	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%d", INT_MAX - 1);
	err = strtoi_err_endptr(tmp_str, NULL, &value);
	check(err == 0);
	check(value == INT_MAX - 1);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%d", INT_MAX);
	err = strtoi_err_endptr(tmp_str, NULL, &value);
	check(err == 0);
	check(value == INT_MAX);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%d", INT_MIN + 1);
	err = strtoi_err_endptr(tmp_str, NULL, &value);
	check(err == 0);
	check(value == INT_MIN + 1);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%d", INT_MIN);
	err = strtoi_err_endptr(tmp_str, NULL, &value);
	check(err == 0);
	check(value == INT_MIN);

	// test errors
	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%ld", (long)INT_MAX + 1);
	err = strtoi_err_endptr(tmp_str, NULL, &value);
	check(err == -ERANGE);
	check(value == INT_MAX);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%ld", (long)INT_MAX + 1000);
	err = strtoi_err_endptr(tmp_str, NULL, &value);
	check(err == -ERANGE);
	check(value == INT_MAX);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%ld", (long)INT_MIN - 1);
	err = strtoi_err_endptr(tmp_str, NULL, &value);
	check(err == -ERANGE);
	check(value == INT_MIN);

	value = 0xffffff;
	snprintf(tmp_str, sizeof(tmp_str), "%ld", (long)INT_MIN - 1000);
	err = strtoi_err_endptr(tmp_str, NULL, &value);
	check(err == -ERANGE);
	check(value == INT_MIN);

	// Test endptr trailing chars
	value = 0xffffff;
	endptr = NULL;
	err = strtoi_err_endptr("123.456", &endptr, &value);
	check(err == 0);
	check(value == 123);
	check(endptr != NULL);
	check(*endptr == '.');

	value = 0xffffff;
	endptr = NULL;
	err = strtoi_err_endptr("123a456", &endptr, &value);
	check(err == 0);
	check(value == 123);
	check(endptr != NULL);
	check(*endptr == 'a');

	value = 0xffffff;
	endptr = NULL;
	err = strtoi_err_endptr("123testing456", &endptr, &value);
	check(err == 0);
	check(value == 123);
	check(endptr != NULL);
	check(*endptr == 't');

	value = 0xffffff;
	endptr = NULL;
	err = strtoi_err_endptr("123-456", &endptr, &value);
	check(err == 0);
	check(value == 123);
	check(endptr != NULL);
	check(*endptr == '-');

	value = 0xffffff;
	endptr = NULL;
	err = strtoi_err_endptr("-123.456", &endptr, &value);
	check(err == 0);
	check(value == -123);
	check(endptr != NULL);
	check(*endptr == '.');

	value = 0xffffff;
	endptr = NULL;
	err = strtoi_err_endptr("-123a456", &endptr, &value);
	check(err == 0);
	check(value == -123);
	check(endptr != NULL);
	check(*endptr == 'a');

	value = 0xffffff;
	endptr = NULL;
	err = strtoi_err_endptr("-123testing456", &endptr, &value);
	check(err == 0);
	check(value == -123);
	check(endptr != NULL);
	check(*endptr == 't');

	value = 0xffffff;
	endptr = NULL;
	err = strtoi_err_endptr("-123-456", &endptr, &value);
	check(err == 0);
	check(value == -123);
	check(endptr != NULL);
	check(*endptr == '-');

}

void test_string_join()
{
	struct list *str_list = NULL;
	char *joined;

	joined = string_join(" long separator ", str_list);
	check(strcmp(joined, "") == 0);
	free(joined);

	str_list = list_prepend_data(str_list, "string3");

	joined = string_join(" long separator ", str_list);
	check(strcmp(joined, "string3") == 0);
	free(joined);

	str_list = list_prepend_data(str_list, "string2");
	str_list = list_prepend_data(str_list, "string1");

	joined = string_join(",", str_list);
	check(strcmp(joined, "string1,string2,string3") == 0);
	free(joined);

	joined = string_join(" long separator ", str_list);
	check(strcmp(joined, "string1 long separator string2 long separator string3") == 0);
	free(joined);

	joined = string_join("", str_list);
	check(strcmp(joined, "string1string2string3") == 0);
	free(joined);

	joined = string_join(NULL, str_list);
	check(strcmp(joined, "string1string2string3") == 0);
	free(joined);

	list_free_list(str_list);
}

int main() {
	test_strtoi_err();
	test_strtoi_err_endptr();
	test_string_join();

	return 0;
}
