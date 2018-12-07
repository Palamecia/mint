#ifndef DEBUG_TOOL_H
#define DEBUG_TOOL_H

#include <string>

namespace mint {

void set_main_module_path(const std::string &path);

std::string get_module_line(const std::string &module, size_t line);

}

#endif // DEBUG_TOOL_H
