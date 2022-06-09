#include "memory/functiontool.h"
#include "memory/casttool.h"
#include <cerrno>

using namespace std;
using namespace mint;

MINT_FUNCTION(mint_errno_get, 0, cursor) {
	FunctionHelper(cursor, 0).returnValue(create_number(errno));
}

MINT_FUNCTION(mint_errno_strerror, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	Reference &error = helper.popParameter();
	helper.returnValue(create_string(strerror(to_integer(cursor, error))));
}
