#ifndef DEBUG_TOOL_H
#define DEBUG_TOOL_H

#include <config.h>
#include <string>

namespace mint {

MINT_EXPORT void set_main_module_path(const std::string &path);

MINT_EXPORT std::string get_module_line(const std::string &module, size_t line);

}

#endif // DEBUG_TOOL_H
