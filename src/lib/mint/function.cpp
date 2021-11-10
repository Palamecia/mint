#include <memory/functiontool.h>
#include <memory/memorytool.h>
#include <memory/operatortool.h>
#include <ast/cursor.h>

using namespace mint;
using namespace std;

MINT_FUNCTION(mint_get_member_infos, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference member = move(helper.popParameter());
	WeakReference object = move(helper.popParameter());

	if (object.data()->format == Data::fmt_object) {
		if (Class::MemberInfo *infos = get_member_infos(object.data<Object>(), member)) {
			helper.returnValue(create_object(infos));
		}
	}
}

MINT_FUNCTION(mint_function_call, 4, cursor) {

	WeakReference args = move(cursor->stack().back());
	cursor->stack().pop_back();

	WeakReference func = move(cursor->stack().back());
	cursor->stack().pop_back();

	WeakReference object = move(cursor->stack().back());
	cursor->stack().pop_back();

	WeakReference infos = move(cursor->stack().back());
	cursor->stack().pop_back();

	int argc = 0;

	cursor->stack().emplace_back(move(object));
	cursor->waitingCalls().emplace(move(func));

	while (optional<WeakReference> &&argv = iterator_next(args.data<Iterator>())) {
		cursor->stack().emplace_back(move(*argv));
		argc++;
	}

	cursor->waitingCalls().top().setMetadata(infos.data<LibObject<Class::MemberInfo>>()->impl->owner);
	call_member_operator(cursor, argc);
}
