#include "system/assert.h"

#if !defined(OS_WINDOWS) && defined(BUILD_TYPE_DEBUG)
#include "system/terminal.h"
#include <cstdio>

void __assert_x_fail (const char *__assertion, const char *__file, unsigned int __line,
					  const char *__function, const char *__where, const char *__what) __THROW {
	mint::term_printf(stderr, "%s: %s\n", __where, __what);
	__assert_fail(__assertion, __file, __line, __function);
}
#endif
