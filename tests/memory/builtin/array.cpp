#include <gtest/gtest.h>
#include <memory/builtin/array.h>
#include <memory/builtin/string.h>
#include <memory/functiontool.h>
#include <memory/operatortool.h>
#include <memory/memorytool.h>
#include <scheduler/processor.h>
#include <ast/abstractsyntaxtree.h>
#include <ast/cursor.h>

using namespace std;
using namespace mint;

#define wait_for_result(cursor) while (1u < cursor->stack().size()) { ASSERT_TRUE(run_step(cursor)); }

TEST(array, join) {

	SharedReference array = create_array({
											 create_string("a"),
											 create_string("b"),
											 create_string("c")
										 });

	Cursor *cursor = AbstractSyntaxTree::instance().createCursor();
	cursor->stack().emplace_back(move(array));
	cursor->stack().emplace_back(create_string(", "));

	ASSERT_TRUE(call_overload(cursor, "join", 1));
	wait_for_result(cursor);

	SharedReference result = move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::fmt_object, result->data()->format);
	ASSERT_EQ(Class::string, result->data<Object>()->metadata->metatype());
	EXPECT_EQ("a, b, c", result->data<String>()->str);
	delete cursor;
}
