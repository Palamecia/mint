#ifndef ASSERT_H
#define ASSERT_H

#include "config.h"

#include <assert.h>

#ifdef BUILD_TYPE_DEBUG
extern void __assert_x_fail (const char *__assertion, const char *__file,
                             unsigned int __line, const char *__function,
                             const char *__where, const char *__what)
__THROW __attribute__ ((__noreturn__));

#define assert_x(expr, where, what) \
        (static_cast <bool> (expr)						\
         ? void (0)							\
         : __assert_x_fail (#expr, __FILE__, __LINE__, __ASSERT_FUNCTION, where, what))
#else
#define assert_x(expr, where, what)
#endif

#endif // ASSERT_H
