/*
 * Assert and print error message if _exp is false.
 */
#define check_msg(_exp, _msg) \
	do {\
		if (!(_exp)) { \
			printf("Test error(%s:%d): %s\n", __FILE__, __LINE__, _msg); \
			exit(EXIT_FAILURE); \
		} 		\
	} while(0) \

#define check(_exp) check_msg(_exp, "failed")
