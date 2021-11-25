#ifndef MINT_ASSERT_H
#define MINT_ASSERT_H

#include "config.h"

#include <cassert>

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

namespace mint {

template <typename Exception, typename Type, typename... Args>
Type *assert_not_null(Type *value, Args&&... args) {
#if 0
	if (UNLIKELY(value == nullptr)) {
		throw Exception(std::forward<Args>(args)...);
	}
#endif
	return value;
}

}

#endif // MINT_ASSERT_H
