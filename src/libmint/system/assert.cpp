#include "system/assert.h"
#include "system/terminal.h"

#include <cstdio>

#ifdef BUILD_TYPE_DEBUG
void __assert_x_fail (const char *__assertion, const char *__file, unsigned int __line,
					  const char *__function, const char *__where, const char *__what) __THROW {
	mint::term_printf(stderr, "%s: %s\n", __where, __what);
	__assert_fail(__assertion, __file, __line, __function);
}
#endif
