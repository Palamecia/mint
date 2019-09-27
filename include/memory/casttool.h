#ifndef CAST_TOOL_H
#define CAST_TOOL_H

#include "memory/builtin/array.h"
#include "memory/builtin/hash.h"

#include <regex>

namespace mint {

class Cursor;

MINT_EXPORT double to_number(Cursor *cursor, SharedReference &ref);
MINT_EXPORT double to_number(Cursor *cursor, SharedReference &&ref);
MINT_EXPORT bool to_boolean(Cursor *cursor, SharedReference &ref);
MINT_EXPORT bool to_boolean(Cursor *cursor, SharedReference &&ref);
MINT_EXPORT std::string to_char(const SharedReference &ref);
MINT_EXPORT std::string to_string(const SharedReference &ref);
MINT_EXPORT std::regex to_regex(SharedReference &ref);
MINT_EXPORT Array::values_type to_array(SharedReference &ref);
MINT_EXPORT Hash::values_type to_hash(Cursor *cursor, SharedReference &ref);

}

#endif // CAST_TOOL_H
