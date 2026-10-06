#ifndef LIB_SYSTEM_ERRNO_H
#define LIB_SYSTEM_ERRNO_H

#include "mint/memory/function_tools.h"
#include "mint/memory/reference.h"
#include "mint/program/cursor.h"
#include <utility>

namespace mint_system {

template<class... Values>
mint::Reference make_success(mint::Cursor& cursor, Values&&... values) {
	return mint::create_iterator_from(cursor, mint::create_number(0), std::forward<Values>(values)...);
}

inline mint::Reference make_failure(mint::Cursor& cursor, int error) {
	return mint::create_iterator_from(cursor, mint::create_number(error));
}

}

#endif // LIB_SYSTEM_ERRNO_H
