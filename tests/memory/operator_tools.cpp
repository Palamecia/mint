#include <gtest/gtest.h>
#include "mint/memory/operator_tools.h"
#include "mint/ast/abstract_syntax_tree.h"
#include "mint/ast/cursor.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/class_tools.h"
#include "mint/memory/data.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/processor.h"
#include "mint/scheduler/scheduler.h"
#include "mint/system/mint_runtime_error.h"

#define WAIT_FOR_RESULT(__cursor) \
	while ((__cursor).call_in_progress()) { \
		ASSERT_EQ(mint::ProcessStatus::paused, mint::run_step(__cursor)); \
	}

#define EXPECT_THROW_WHAT(statement, expected_exception, msg) \
	try { \
		statement; \
		FAIL() << "Expected exception: " #expected_exception "\n  Actual: no exception thrown"; \
	} \
	catch (const expected_exception& e) { \
		EXPECT_STREQ(msg, e.what()); \
	} \
	catch (...) { \
		FAIL() << "Expected exception: " #expected_exception "\n  Actual: different exception type thrown"; \
	}

TEST(operator_tools, call_overload) {

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
	EXPECT_THROW_WHAT(mint::call_overload(cursor, "+", 1), mint::MintRuntimeError,
	    "invalid use of class 'Foo' with operator '+'(1)");
	cursor.stack().clear();
}
