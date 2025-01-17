#include <gtest/gtest.h>
#include <mint/memory/operatortool.h>
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/cursor.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/functiontool.h"
#include "mint/scheduler/processor.h"

using namespace mint;

#define WAIT_FOR_RESULT(cursor) \
	while (1u < cursor->stack().size()) { \
		ASSERT_TRUE(run_step(cursor)); \
	}

TEST(operatortool, call_overload) {

	GarbageCollector::instance();
	AbstractSyntaxTree ast;
	std::unique_ptr<Cursor> cursor(ast.create_cursor());

	cursor->stack().emplace_back(create_string("foo"));
	cursor->stack().emplace_back(create_string("bar"));
	EXPECT_TRUE(call_overload(cursor.get(), "+", 1));
	WAIT_FOR_RESULT(cursor.get());

	EXPECT_EQ(Data::FMT_OBJECT, cursor->stack().back().data()->format);
	EXPECT_EQ(Class::STRING, cursor->stack().back().data<Object>()->metadata->metatype());
	EXPECT_STREQ("foobar", cursor->stack().back().data<String>()->str.c_str());
	cursor->stack().clear();

	cursor->stack().emplace_back(create_string("foo"));
	cursor->stack().emplace_back(create_string("bar"));
	EXPECT_FALSE(call_overload(cursor.get(), "#", 1));
	cursor->stack().clear();

	cursor->stack().emplace_back(WeakReference::create<String>());
	cursor->stack().emplace_back(create_string("bar"));
	ASSERT_DEATH(call_overload(cursor.get(), "+", 1), "invalid use of class 'string' in an operation");
	cursor->stack().clear();
}
