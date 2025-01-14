#include <gtest/gtest.h>
#include <mint/memory/builtin/array.h>
#include <mint/memory/builtin/string.h>
#include <mint/memory/functiontool.h>
#include <mint/memory/operatortool.h>
#include <mint/memory/memorytool.h>
#include <mint/scheduler/processor.h>
#include <mint/ast/abstractsyntaxtree.h>
#include <mint/ast/cursor.h>

using namespace mint;

#define wait_for_result(cursor) \
	while (1u < cursor->stack().size()) { \
		ASSERT_TRUE(run_step(cursor)); \
	}

TEST(array, join) {

	AbstractSyntaxTree ast;

	WeakReference array = create_array({
		create_string("a"),
		create_string("b"),
		create_string("c"),
	});

	Cursor *cursor = ast.create_cursor();
	cursor->stack().emplace_back(std::forward<Reference>(array));
	cursor->stack().emplace_back(create_string(", "));

	ASSERT_TRUE(call_overload(cursor, "join", 1));
	wait_for_result(cursor);

	WeakReference result = std::move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::FMT_OBJECT, result.data()->format);
	ASSERT_EQ(Class::STRING, result.data<Object>()->metadata->metatype());
	EXPECT_EQ("a, b, c", result.data<String>()->str);
	delete cursor;
}
