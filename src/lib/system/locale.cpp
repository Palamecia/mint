#include <memory/functiontool.h>
#include <locale>

using namespace std;
using namespace mint;

MINT_FUNCTION(mint_locale_current_name, 0, cursor) {
	FunctionHelper helper(cursor, 0);
	helper.returnValue(create_string(locale().name()));
}
