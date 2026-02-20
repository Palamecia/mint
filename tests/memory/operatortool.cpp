#include <gtest/gtest.h>
#include "mint/memory/operatortool.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/cursor.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/data.h"
#include "mint/memory/functiontool.h"
#include "mint/scheduler/processor.h"
#include "mint/scheduler/scheduler.h"

#define WAIT_FOR_RESULT(__cursor) \
	while (1uz < (__cursor).stack().size()) { \
		ASSERT_TRUE(run_step(__cursor)); \
	}

TEST(operatortool, call_overload) {

	mint::Scheduler scheduler({});
	mint::AbstractSyntaxTree& ast = scheduler.ast();
	auto cursor = mint::Cursor(ast);

	cursor.stack().emplace_back(mint::create_string(scheduler.ast(), "foo"));
	cursor.stack().emplace_back(mint::create_string(scheduler.ast(), "bar"));
	EXPECT_TRUE(mint::call_overload(cursor, "+", 1));
	WAIT_FOR_RESULT(cursor);

	EXPECT_EQ(mint::Data::Format::object, cursor.stack().back().data().format());
	EXPECT_EQ(mint::Class::Metatype::string, cursor.stack().back().data<mint::Object>().metadata.metatype());
	EXPECT_EQ("foobar", cursor.stack().back().data<mint::String>().str);
	cursor.stack().clear();

	cursor.stack().emplace_back(mint::create_string(scheduler.ast(), "foo"));
	cursor.stack().emplace_back(mint::create_string(scheduler.ast(), "bar"));
	EXPECT_FALSE(mint::call_overload(cursor, "#", 1));
	cursor.stack().clear();

	cursor.stack().emplace_back(mint::create_alias(mint::StringClass::instance(scheduler.ast())));
	cursor.stack().emplace_back(mint::create_string(scheduler.ast(), "bar"));
	ASSERT_DEATH(mint::call_overload(cursor, "+", 1), "invalid use of class 'string' in an operation");
	cursor.stack().clear();
}
