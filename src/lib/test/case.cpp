#include <memory/functiontool.h>
#include <debug/debugtool.h>
#include <ast/cursor.h>

using namespace mint;
using namespace std;

MINT_FUNCTION(mint_test_case_line_infos, 0, cursor) {
	cursor->exitCall();
	LineInfoList call_stack = cursor->dump();
	LineInfo line_info = call_stack.at(1);
	cursor->stack().emplace_back(create_string(line_info.toString() + ":\n" + get_module_line(line_info.moduleName(), line_info.lineNumber())));
}
