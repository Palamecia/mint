#include <memory/functiontool.h>
#include <ast/cursor.h>

using namespace mint;
using namespace std;

MINT_FUNCTION(mint_test_case_line_infos, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	vector<string> call_stack = cursor->dump();
	helper.returnValue(create_string(call_stack.at(1)));
}
