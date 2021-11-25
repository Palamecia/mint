#include <memory/functiontool.h>
#include <memory/casttool.h>
#include <locale>

using namespace std;
using namespace mint;

/**
@see https://man7.org/linux/man-pages/man5/locale.5.html
@see https://docs.microsoft.com/en-us/windows/win32/intl/national-language-support
*/

MINT_FUNCTION(mint_locale_current_name, 0, cursor) {
	FunctionHelper helper(cursor, 0);
	helper.returnValue(create_string(locale().name()));
}

MINT_FUNCTION(mint_locale_set_current_name, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	Reference &name = helper.popParameter();
	helper.returnValue(create_boolean(setlocale(LC_ALL, to_string(name).c_str()) != nullptr));
}
