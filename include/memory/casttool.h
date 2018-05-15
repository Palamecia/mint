#ifndef CAST_TOOL_H
#define CAST_TOOL_H

#include "memory/builtin/array.h"
#include "memory/builtin/hash.h"

#include <regex>

namespace mint {

class Cursor;

MINT_EXPORT double to_number(Cursor *cursor, const Reference &ref);
MINT_EXPORT bool to_boolean(Cursor *cursor, const Reference &ref);
MINT_EXPORT std::string to_char(const Reference &ref);
MINT_EXPORT std::string to_string(const Reference &ref);
MINT_EXPORT std::regex to_regex(const Reference &ref);
MINT_EXPORT Array::values_type to_array(const Reference &ref);
MINT_EXPORT Hash::values_type to_hash(Cursor *cursor, const Reference &ref);

}

#endif // CAST_TOOL_H
