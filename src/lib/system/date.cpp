#include <memory/functiontool.h>
#include <chrono>

using namespace std;
using namespace mint;

MINT_FUNCTION(mint_date_current_time, 0, cursor) {
	FunctionHelper helper(cursor, 0);
	helper.returnValue(create_object(new chrono::milliseconds(chrono::system_clock::now().time_since_epoch().count())));
}

MINT_FUNCTION(mint_date_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference milliseconds = move(helper.popParameter());

	delete milliseconds->data<LibObject<chrono::milliseconds>>()->impl;
}

MINT_FUNCTION(mint_date_time_to_milliseconds, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference milliseconds = move(helper.popParameter());

	helper.returnValue(create_number(static_cast<double>(milliseconds->data<LibObject<chrono::milliseconds>>()->impl->count())));
}

MINT_FUNCTION(mint_date_milliseconds_to_time, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference number = move(helper.popParameter());

	helper.returnValue(create_object(new chrono::milliseconds(static_cast<uintmax_t>(number->data<Number>()->value))));
}
