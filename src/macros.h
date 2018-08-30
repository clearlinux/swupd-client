#ifndef __INCLUDE_GUARD_MACROS_H
#define __INCLUDE_GUARD_MACROS_H

#define ON_NULL_ABORT(var) \
	if (!var) {        \
		abort();   \
	}

#endif
