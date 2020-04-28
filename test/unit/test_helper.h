#include <stdlib.h>
#include <stdio.h>

#define print_error(_msg) printf("Test error(%s:%d): %s\n", __FILE__, __LINE__, _msg);

/*
 * Assert and print error message if _exp is false.
 */
#define check_msg(_exp, _msg)			\
	do {					\
		if (!(_exp)) {			\
			print_error(_msg);	\
			exit(EXIT_FAILURE);	\
		}				\
	} while(0)

/*
 * Assert and print error message if _exp is false.
 * Goto to label instead of exiting.
 */
#define check_msg_goto(_exp, _msg, _label)	\
	do {					\
		if (!(_exp)) {			\
			print_error(_msg);	\
			goto _label;		\
		}				\
	} while(0)

#define check(_exp) check_msg(_exp, "failed")
#define check_goto(_exp, _label) check_msg_goto(_exp, "failed", _label)
