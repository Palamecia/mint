#include <gtest/gtest.h>
#include "mint/memory/operatortool.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/cursor.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/classtool.h"
#include "mint/memory/data.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/processor.h"
#include "mint/scheduler/scheduler.h"

#define WAIT_FOR_RESULT(__cursor) \
	while (cursor.call_in_progress()) { \
		ASSERT_TRUE(run_step(__cursor)); \
	}

TEST(operatortool, call_overload) {

	auto scheduler = mint::Scheduler({});
	auto cursor = mint::Cursor(scheduler.ast());

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

	cursor.stack().emplace_back(mint::create_alias(mint::create_class(scheduler.ast(), "Foo", {{"+", {}}})));
	cursor.stack().emplace_back(mint::create_string(scheduler.ast(), "bar"));
	ASSERT_DEATH(mint::call_overload(cursor, "+", 1), "invalid use of class 'Foo' with operator '\\+'\\(1\\)");
	cursor.stack().clear();
}
