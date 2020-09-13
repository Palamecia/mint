#include "memory/functiontool.h"
#include "memory/builtin/hash.h"

using namespace mint;
using namespace std;

MINT_FUNCTION(mint_operator_hash_key_compare, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference right = move(helper.popParameter());
	SharedReference left = move(helper.popParameter());

	static constexpr Hash::compare comparator;
	helper.returnValue(create_boolean(comparator(left, right)));
}
