#include <memory/garbagecollector.h>
#include <memory/functiontool.h>

using namespace mint;

MINT_FUNCTION(mint_garbage_collector_collect, 0, cursor) {
	FunctionHelper helper(cursor, 0);
	helper.returnValue(create_number(GarbageCollector::instance().collect()));
}
