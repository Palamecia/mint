#ifndef MINT_DEBUGTOOL_H
#define MINT_DEBUGTOOL_H

#include "ast/node.h"

#include <string>

namespace mint {

class Cursor;

MINT_EXPORT void set_main_module_path(const std::string &path);

MINT_EXPORT std::string get_module_line(const std::string &module, size_t line);

MINT_EXPORT void dump_command(size_t offset, Node::Command command, Cursor *cursor, std::ostream &stream);

}

#endif // MINT_DEBUGTOOL_H
